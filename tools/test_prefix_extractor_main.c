#include "PrefixExtractor.h"

int main(int argc, char **argv) {
    if (argc != 3) return 2;
    return madeira_extract_prefix_tgz(argv[1], argv[2]) == 0 ? 0 : 1;
}
