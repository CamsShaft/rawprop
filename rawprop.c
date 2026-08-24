
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <inttypes.h>
#include <sys/stat.h>

#define PROP_VALUE_MAX 92
#define PROP_AREA_MAGIC 0x504f5250
#define PROP_AREA_VERSION 0xfc6ed0ab
#define kLongFlag (1 << 16)
#define AREA_SIZE (128 * 1024)
#define BUF_SIZE 1024

struct prop_trie_node {
  uint32_t namelen;
  uint32_t prop;
  uint32_t left;
  uint32_t right;
  uint32_t children;
  char name[0];
};

struct prop_info {
  uint32_t serial;
  union {
    char value[PROP_VALUE_MAX];
    struct {
      char error_message[56];
      uint32_t offset;
    } long_property;
  };
  char name[0];
};

struct prop_area {
  uint32_t bytes_used;
  uint32_t serial;
  uint32_t magic;
  uint32_t version;
  uint32_t reserved[28];
  char data_[0];
};

int cmp_prop_name(const char *one, uint32_t one_len, const char *two, uint32_t two_len) {
  if (one_len < two_len) return -1;
  if (one_len > two_len) return 1;
  return strncmp(one, two, one_len);
}

const struct prop_info *find_property(struct prop_area *pa, const char *name) {
  char *data = pa->data_;
  uint32_t current_offset = 0;
  const char *remaining_name = name;
  while (1) {
    const char *sep = strchr(remaining_name, '.');
    uint32_t substr_size = sep ? (uint32_t)(sep - remaining_name) : (uint32_t)strlen(remaining_name);
    if (!substr_size) return NULL;

    uint32_t node_offset = ((struct prop_trie_node*)&data[current_offset])->children;
    while (1) {
      if (node_offset == 0) return NULL;
      struct prop_trie_node *node = (struct prop_trie_node*)&data[node_offset];
      int ret = cmp_prop_name(remaining_name, substr_size, node->name, node->namelen);
      if (ret == 0) {
        current_offset = node_offset;
        break;
      }
      node_offset = (ret < 0) ? node->left : node->right;
    }
    if (!sep) break;
    remaining_name = sep + 1;
  }
  uint32_t prop_offset = ((struct prop_trie_node*)&data[current_offset])->prop;
  if (prop_offset == 0) return NULL;
  return (const struct prop_info*)&data[prop_offset];
}

const char *get_prop_value(const struct prop_info *pi) {
  uint32_t serial = pi->serial;
  if (serial & kLongFlag) {
    return (const char *)pi + pi->long_property.offset;
  } else {
    return pi->value;
  }
}

void set_prop_value(struct prop_info *pi, const char *new_value) {
  memset(pi->value, 0, PROP_VALUE_MAX);
  uint32_t len = strlen(new_value);
  pi->serial = (pi->serial & 0x00FFFFFF & ~kLongFlag) | (len << 24);
  strncpy(pi->value, new_value, PROP_VALUE_MAX - 1);
}

struct prop_area *map_area(const char *filename, int write_mode) {
  int flags = write_mode ? O_RDWR : O_RDONLY;
  int prot = write_mode ? (PROT_READ | PROT_WRITE) : PROT_READ;
  int map_flags = MAP_SHARED;

  int fd = open(filename, flags);
  if (fd < 0) {
    perror("open");
    return NULL;
  }
  void *map = mmap(NULL, AREA_SIZE, prot, map_flags, fd, 0);
  close(fd);
  if (map == MAP_FAILED) {
    perror("mmap");
    return NULL;
  }
  struct prop_area *pa = (struct prop_area *)map;
  if (pa->magic != PROP_AREA_MAGIC || pa->version != PROP_AREA_VERSION) {
    fprintf(stderr, "Invalid magic (%08x != %08x) or version (%08x != %08x)\n",
            pa->magic, PROP_AREA_MAGIC, pa->version, PROP_AREA_VERSION);
    munmap(map, AREA_SIZE);
    return NULL;
  }
  return pa;
}

int chprop(const char *ctx, const char *name, const char *value) {
  struct stat st;
  if (stat(ctx, &st) < 0) {
    perror(ctx);
    return 1;
  }
  struct prop_area *pa = map_area(ctx, !!value);
  if (!pa) return 1;
  const struct prop_info *pi = find_property(pa, name);
  int ret = 1;
  if (!pi) {
    fprintf(stderr, "%s: not found in %s\n", name, ctx);
    if (!value) printf("\n");
  } else {
    ret = 0;
    if (value) {
      set_prop_value((struct prop_info*)pi, value);
    } else {
      printf("%s\n", get_prop_value(pi));
    }
  }
  munmap(pa, AREA_SIZE);
  if (value) {
    struct timespec ts[2] = { st.st_atim, st.st_mtim };
    utimensat(AT_FDCWD, ctx, ts, 0);
  }
  return ret;
}

int main(int argc, char **argv) {
  if (argc == 2 && argv[1][0] == '-') {
    char line[BUF_SIZE];
    while (fgets(line, sizeof(line), stdin)) {
      char *p;
      chprop(strtok(line, " "), strtok(NULL, " "), (p=strtok(NULL, "\n"))?p:"");
    }
    return 0;
  }
  if (argc < 3) {
    fprintf(stderr, "%s /dev/__properties__/... <property> [new_value]\n", argv[0]);
    return 1;
  }
  return chprop(argv[1], argv[2], argv[3]);
}

