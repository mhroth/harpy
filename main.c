/* Copyright (c) 2015, Section 6. All Rights Reserved. */

#include <asoundlib.h>  // alsa
#include <arpa/inet.h>  // network
#include <pthread.h>    // threads
#include <stdatomic.h>
#include <sys/socket.h> // sockets
#include <time.h>       // nanosleep, clock_gettime
#include "tinyosc.h"    // OSC support

#define BLOCK_SIZE 256
#define NUM_OUTPUT_CHANNELS 2
#define SEC_TO_NS 1000000000

static atomic_flag networkThreadShouldContinue;

static volatile bool _keepRunning = true;

// http://stackoverflow.com/questions/4217037/catch-ctrl-c-in-c
static void sigintHandler(int x) {
  printf("Termination signal received.\n"); // handle Ctrl+C
  _keepRunning = false;
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

static void hv_printHook(double timestamp, const char *name, const char *s,
    void *userData) {
  printf("[@h %.3fms] %s: %s\n", timestamp, name, s);
}

static void hv_sendHook(double timestamp, const char *receiverName,
    const HvMessage *m, void *userData) {
  // TODO(mhroth)
}

static void handleOscMessage(tosc_message *osc, const uint64_t timetag) {
  if (!strncmp(tosc_getAddress(osc), "/slot", 5)) {
    // address looks like "/slot/0/gain"
    const int i = tosc_getAddress(osc)[6] - '0';
    pthread_mutex_lock(m->lock);
    hv_vscheduleMessageForReceiver(m->mods[i], tosc_getAddress(osc)+8, 0.0,
        "f", tosc_getNextFloat(osc));
    pthread_mutex_unlock(m->lock);
  } else if (!strcmp(tosc_getAddress(osc), "/admin")) {
    if (!strcmp(tosc_getNextString(osc), "quit")) {
      _keepRunning = false;
    }
  }
}

static void *network_run(void *x) {
  Modules *m = (Modules *) x;

  struct sockaddr_in sin;
  int len = 0;
  int sa_len = sizeof(struct sockaddr_in);
  tosc_tinyosc osc;
  char buffer[2048];

  // prepare the receive socket
  const int fd_receive = socket(AF_INET, SOCK_DGRAM, 0);
  // fcntl(fd, F_SETFL, O_NONBLOCK); // set the socket to non-blocking
  sin.sin_family = AF_INET;
  sin.sin_port = htons(2015);
  sin.sin_addr.s_addr = INADDR_ANY;
  bind(fd, (struct sockaddr *) &sin, sizeof(struct sockaddr_in));

  while (_keepRunning) {
    // set up structs for select
    fd_set rfds;
    FD_ZERO(&rfds);
    FD_SET(fd_receive, &rfds);

    struct timeval tv;
    tv.tv_sec = 1; // wait up to 1 second
    tv.tv_usec = 0;

    // listen to the socket for any responses
    if (select(1, &rfds, NULL, NULL, &tv) > 0) {
      while ((len = recvfrom(fd_receive, buffer, sizeof(buffer), 0, (struct sockaddr *) &sin, (socklen_t *) &sa_len)) > 0) {
        if (tosc_isBundle(buffer)) {
          tosc_bundle bundle;
          tosc_readBundle(&bundle, buffer, len);
          const uint64_t timetag = tosc_getTimetag(&bundle);
          tosc_message osc;
          while (tosc_getNextMessage(&bundle, &osc)) {
            handleOscMessage(&osc, timetag);
          }
        } else {
          tosc_message osc;
          tosc_readMessage(&osc, buffer, len);
          handleOscMessage(&osc, 0L);
        }
      }
    }
  }

  return NULL;
}

typedef struct Modules {
  void *mods[4];
  pthread_mutex_t lock;
} Modules;

// http://www.alsa-project.org/alsa-doc/alsa-lib/_2test_2pcm_min_8c-example.html
int main() {
  signal(SIGINT, &sigintHandler); // register the SIGINT handler

  Modules m;
  pthread_mutex_t(&m.lock, NULL);

  struct timespec tick, tock, diff_tick;

  // setup sound output
  snd_pcm_t *alsa;
  snd_pcm_open(&alsa, "RPistorius", SND_PCM_STREAM_PLAYBACK, SND_PCM_ASYNC);
  snd_pcm_set_params(alsa,
      SND_PCM_FORMAT_FLOAT_LE ,
      SND_PCM_ACCESS_RW_NONINTERLEAVED,
      NUM_OUTPUT_CHANNELS, // stereo output
      48000,   // 48KHz sampling rate
      1,       // 0 = disallow alsa-lib resample stream, 1 = allow resampling
      500000); // required overall latency in us

  void *heavySlot0 = hv_firehelix_new_with_pool(HEAVY_SAMPLE_RATE, 100);
  hv_setPrintHook(heavySlot0, &hv_printHook);
  hv_setSendHook(heavySlot0, &hv_sendHook);

  // https://computing.llnl.gov/tutorials/pthreads/
  pthread_t networkThread = 0;
  pthread_create(&networkThread, NULL, &network_run, &m);

  // the audio loop
  float audioBuffer[NUM_OUTPUT_CHANNELS * BLOCK_SIZE];
  while (_keepRunning) {
    pthread_mutex_lock(&m.lock);
    clock_gettime(CLOCK_REALTIME, &tick);
    // TODO(mhroth): process Heavy
    hv_heavy_process_inline(m.mods[0], NULL, audioBuffer, BLOCK_SIZE);
    hv_heavy_process_inline(m.mods[1], NULL, audioBuffer, BLOCK_SIZE);
    hv_heavy_process_inline(m.mods[2], NULL, audioBuffer, BLOCK_SIZE);
    hv_heavy_process_inline(m.mods[3], NULL, audioBuffer, BLOCK_SIZE);
    clock_gettime(CLOCK_REALTIME, &tock);
    pthread_mutex_unlock(&m.lock);
    timespec_subtract(&diff_tick, &tock, &tick);
    const int64_t elapsed_ns = (((int64_t) diff_tick.tv_sec) * SEC_TO_NS) + diff_tick.tv_nsec;

    snd_pcm_sframes_t frames = snd_pcm_writen(alsa, audioBuffer, sizeof(audioBuffer));
    if (frames < 0) frames = snd_pcm_recover(handle, audioBuffer, 0);
    if (frames < 0) printf("ALSA: snd_pcm_writen failed: %s\n", snd_strerror(frames));
  }

  // wait until the network thread has quit
  pthread_join(networkThread, NULL);

  pthread_mutex_destroy(&m.lock);

  // shut down the audio
  snd_pcm_close(alsa);

  // free all of the slots
  hv_firehelix_free(heavySlot0);
  return 0;
}
