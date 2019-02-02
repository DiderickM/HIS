struct averageCounter{
  uint16_t *samples;
  uint16_t sample_size;
  uint8_t counter;

  averageCounter(uint16_t size) {
    counter = 0;
    sample_size = size;
    samples = (uint16_t*) malloc(sizeof(uint16_t) * sample_size); //reserver memory for samples
  }

  bool setSample(uint16_t val) {//load sample in
    if (counter < sample_size) { //there is free space for the space
      samples[counter++] = val;
      return true;
    }
    else {
      counter = 0; //there is no free space for the sample
      return false;
    }
  }

  int computeAverage() {
    int accumulator = 0;
    for (int i = 0; i < sample_size; i++) {
      accumulator += samples[i];
    }
    return (int)(accumulator / sample_size);
  }

};

struct heartbeat_message { //send this over
  uint32_t client_id;
  uint32_t chk;
};
