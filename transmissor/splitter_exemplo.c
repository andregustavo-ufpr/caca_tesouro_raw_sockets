#include "transmissor.h"

int main() {
  int bytes;
  int count;
  unsigned char **data = split_file("./examplefile", &bytes, &count);

  int bytesprinted = 0;
  for (int i = 0; i < count; ++i) {
    int bytestoprintt = bytes - bytesprinted;
    printf("\nbuffer %d:\n", i);
    printf("%.*s", bytestoprintt >= 127 ? 127 : bytestoprintt, data[i]);

    // this isnt correct for the last iteration, but at that point it wont be
    // needed anymore
    bytesprinted += 127;
  }
}
