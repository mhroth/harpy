/* Copyright (c) 2015, Martin Roth (mhroth@gmail.com). All Rights Reserved. */

#include <alsa/asoundlib.h>  // alsa
#include <arpa/inet.h>       // network
#include <pthread.h>         // threads
#include <stdatomic.h>
#include <sys/socket.h>      // sockets
#include <stdio.h>
#include <sys/time.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>          // close
#include <ifaddrs.h>

#include "tinyosc/tinyosc.h" // OSC support

// heavy slots
#include "heavy/rpis_osc/Heavy_rpis_osc.h"

#define SAMPLE_RATE 48000
#define BLOCK_SIZE 256
#define NUM_OUTPUT_CHANNELS 2
#define SEC_TO_NS 1000000000L

#define ALSA_DEVICE "plughw:CARD=ALSA,DEV=0"

static volatile bool _keepRunning = true;

// http://stackoverflow.com/questions/4217037/catch-ctrl-c-in-c
static void sigintHandler(int x) {
  _keepRunning = false; // handle Ctrl+C
}

static void timespec_subtract(struct timespec *result, struct timespec *end, struct timespec *start) {
  if (end->tv_nsec < start->tv_nsec) {
    result->tv_sec = end->tv_sec - start->tv_sec - 1;
    result->tv_nsec = SEC_TO_NS + end->tv_nsec - start->tv_nsec;
  } else {
    result->tv_sec = end->tv_sec - start->tv_sec;
    result->tv_nsec = end->tv_nsec - start->tv_nsec;
  }
}

static void hv_printHook(
    double timestamp, const char *name, const char *s, void *userData) {
  printf("[@h %.3fms] %s: %s\n", timestamp, name, s);
}

static void hv_sendHook(double timestamp, const char *receiverName,
    const HvMessage *m, void *userData) {
  // TODO(mhroth)
}

static void printIpForInterface(const char *ifName) {
  char host[INET_ADDRSTRLEN];
  struct ifaddrs *ifaddr = NULL;
  getifaddrs(&ifaddr);
  for (struct ifaddrs *ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
    if (!strcmp(ifa->ifa_name, ifName)) {
      if (ifa->ifa_addr->sa_family == AF_INET) {
        struct sockaddr_in *sa = (struct sockaddr_in *) ifa->ifa_addr;
        inet_ntop(AF_INET, &(sa->sin_addr), host, INET_ADDRSTRLEN);
        printf("r.pistorius listening on osc://%s:9000\n", host);
        break;
      }
    }
  }
  freeifaddrs(ifaddr);
}

typedef struct Modules {
  void *mods[4];
  pthread_mutex_t lock;
} Modules;

static void handleOscMessage(tosc_message *osc, const uint64_t timetag, Modules *m) {
  if (!strcmp(tosc_getAddress(osc), "/1/fader1")) {
    hv_sendFloatToReceiver(m->mods[0], "freq", tosc_getNextFloat(osc));
  } else if (!strcmp(tosc_getAddress(osc), "/admin")) {
    if (!strcmp(tosc_getNextString(osc), "quit")) {
      sigintHandler(SIGINT);
    }
  } else {
    printf("Unknown message: "); tosc_printMessage(osc);
  }
}

// the network thread
static void *network_run(void *x) {
  assert(x != NULL);
  Modules *m = (Modules *) x;

  struct sockaddr_in sin;
  int len = 0;
  char buffer[2*1024]; // 2kB receive buffer

  // prepare the receive socket
  const int fd_receive = socket(AF_INET, SOCK_DGRAM, 0);
  assert(fd_receive > 0);
  sin.sin_family = AF_INET;
  sin.sin_port = htons(9000);
  sin.sin_addr.s_addr = INADDR_ANY;
  bind(fd_receive, (struct sockaddr *) &sin, sizeof(struct sockaddr_in));
  printIpForInterface("eth0");

  tosc_bundle bundle;
  tosc_message osc;

  while (_keepRunning) {
    // set up structs for select
    fd_set rfds;
    FD_ZERO(&rfds);
    FD_SET(fd_receive, &rfds);

    struct timeval tv;
    tv.tv_sec = 1; // wait up to 1 second
    tv.tv_usec = 0;

    // listen to the socket for any responses
    if (select(fd_receive+1, &rfds, NULL, NULL, &tv) > 0) {
      size_t sa_len = sizeof(struct sockaddr_in);
      if ((len = recvfrom(fd_receive, buffer, sizeof(buffer), 0, (struct sockaddr *) &sin, (socklen_t *) &sa_len)) > 0) {
        if (tosc_isBundle(buffer)) {
          tosc_parseBundle(&bundle, buffer, len);
          const uint64_t timetag = tosc_getTimetag(&bundle);
          // all bundle message are executed simultaneously in heavy
          pthread_mutex_lock(m->lock);
          while (tosc_getNextMessage(&bundle, &osc)) {
            handleOscMessage(&osc, timetag, m);
          }
          pthread_mutex_unlock(m->lock);
        } else {
          tosc_parseMessage(&osc, buffer, len);
          pthread_mutex_lock(m->lock);
          handleOscMessage(&osc, 0L, m);
          pthread_mutex_unlock(m->lock);
        }
      }
    }
  }

  // close the OSC socket
  close(fd_receive);

  return NULL;
}

// http://www.alsa-project.org/alsa-doc/alsa-lib/_2test_2pcm_min_8c-example.html
// sudo amixer cset numid=3 1
int main() {
  signal(SIGINT, &sigintHandler); // register the SIGINT handler

  // create the modules (and initialise the lock)
  Modules m;
  pthread_mutex_init(&m.lock, NULL);

  struct timespec tick, tock;

  // setup sound output
  // list all devices: $ aplay -L
  snd_pcm_t *alsa;
  snd_pcm_open(&alsa, ALSA_DEVICE, SND_PCM_STREAM_PLAYBACK, 0); // 0 > blocking
  snd_pcm_set_params(alsa,
      SND_PCM_FORMAT_FLOAT_LE ,
      SND_PCM_ACCESS_RW_NONINTERLEAVED,
      NUM_OUTPUT_CHANNELS, // stereo output
      SAMPLE_RATE, // 48KHz sampling rate
      1,           // 0 = disallow alsa-lib resample stream, 1 = allow resampling
      (unsigned int) ((SEC_TO_NS/((double) SAMPLE_RATE))*BLOCK_SIZE)); // required overall latency in us

  {
    snd_pcm_uframes_t buffer_size;
    snd_pcm_uframes_t period_size;
    snd_pcm_get_params(alsa, &buffer_size, &period_size);
    printf("ALSA:\n  buffer size: %lu\n  period size: %lu\n", buffer_size, period_size);
  }

  // initialise all heavy slots
  m.mods[0] = hv_rpis_osc_new(SAMPLE_RATE);
  hv_setPrintHook(m.mods[0], &hv_printHook);
  hv_setSendHook(m.mods[0], &hv_sendHook);
  assert(hv_getNumOutputChannels(m.mods[0]) == NUM_OUTPUT_CHANNELS);
/*
  Hv_mixer *hv_mixer = hv_mixer_new(SAMPLE_RATE);
  hv_setPrintHook(hv_mixer, &hv_printHook);
  hv_setSendHook(hv_mixer, &hv_sendHook);
  assert(hv_getNumOutputChannels(hv_mixer) == NUM_OUTPUT_CHANNELS);
*/
  // create and start the network thread
  // https://computing.llnl.gov/tutorials/pthreads/
  pthread_t networkThread = 0;
  pthread_create(&networkThread, NULL, &network_run, &m);

  // the audio loop
  float *audioBuffer[2] = {
    (float *) alloca(BLOCK_SIZE*sizeof(float)),
    (float *) alloca(BLOCK_SIZE*sizeof(float))
  };
  while (_keepRunning) {
    // process Heavy
    clock_gettime(CLOCK_REALTIME, &tick);
    pthread_mutex_lock(&m.lock);
    hv_rpis_osc_process(m.mods[0], NULL, audioBuffer, BLOCK_SIZE);
    // hv_bass_process_inline(m.mods[1], NULL, audioBuffer+(1 * NUM_OUTPUT_CHANNELS), BLOCK_SIZE);
    // hv_bass_process_inline(m.mods[2], NULL, audioBuffer+(2 * NUM_OUTPUT_CHANNELS), BLOCK_SIZE);
    // hv_bass_process_inline(m.mods[3], NULL, audioBuffer+(3 * NUM_OUTPUT_CHANNELS), BLOCK_SIZE);
    // hv_mixer_process_inline(hv_mixer, audioBuffer, audioBuffer, BLOCK_SIZE);
    pthread_mutex_unlock(&m.lock);
    clock_gettime(CLOCK_REALTIME, &tock);
#if PRINT_PERF
    struct timespec diff_tock;
    timespec_subtract(&diff_tock, &tock, &tick);
    const int64_t elapsed_ns = (((int64_t) diff_tock.tv_sec) * SEC_TO_NS) + diff_tock.tv_nsec;
    printf("%llins (%0.3f%%CPU)\n",
        elapsed_ns,
        100.0*elapsed_ns/((SEC_TO_NS/((double) SAMPLE_RATE))*BLOCK_SIZE));
#endif // PRINT_PERF

    snd_pcm_sframes_t frames = snd_pcm_writen(alsa, (void **) audioBuffer, BLOCK_SIZE);
    if (frames < 0) {
      frames = snd_pcm_recover(alsa, frames, 0);
      if (frames < 0) printf("ALSA: snd_pcm_writen failed: %s\n", snd_strerror(frames));
    }
  }

  // wait until the network thread has quit
  pthread_join(networkThread, NULL);

  // destroy the lock
  pthread_mutex_destroy(&m.lock);

  // shut down the audio
  snd_pcm_close(alsa);

  // free heavy slots
  hv_rpis_osc_free(m.mods[0]);
  // hv_mixer_free(hv_mixer);

  return 0;
}
