#include <iostream>
#include <memory>
#include <vector>

#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/uio.h>

#include <fcntl.h>
#include <unistd.h>
#include <linux/fs.h>

#define isblk S_ISBLK
#define isreg S_ISREG

// block size
constexpr int BLOCK_SIZE_ = 4096;

size_t getFileSize(int fd) {
  std::shared_ptr<struct stat> st = std::make_shared<struct stat>();

  if (fstat(fd, st.get()) < 0) {
    return -1;
  }

  if (isblk(st->st_mode)) {
    unsigned long long bytes;
    if (ioctl(fd, BLKGETSIZE64, &bytes)) {
      return -1;
    }
    return bytes;
  } else if (isreg(st->st_mode)) {
    return st->st_size;
  }
  return -1;
}

int cat(const char *file_name) {
  int fd = open(file_name, O_RDONLY);
  if (fd < 0) {
    perror("open");
    return 1;
  }

  size_t file_size = getFileSize(fd);
  size_t byte_rem = file_size;
  std::cout << file_name << ": " << file_size << std::endl;

  std::vector<struct iovec> ivs;
  while (byte_rem) {
    size_t bytes_to_read = byte_rem > BLOCK_SIZE_ ? BLOCK_SIZE_ : byte_rem;

    void *buf;
    if (posix_memalign(&buf, BLOCK_SIZE_, BLOCK_SIZE_)) {
      std::cout << "posix error" << std::endl;
      return -1;
    }

    struct iovec iv {
      .iov_base = buf, .iov_len = bytes_to_read,
    };
    ivs.push_back(iv);
    byte_rem -= bytes_to_read;
  }

  int ret = readv(fd, ivs.data(), ivs.size());
  if (ret < 0) {
    std::cout << "ret error" << std::endl;
    return -1;
  }

  for (int i = 0; i < ivs.size(); ++i) {
    const std::string b(static_cast<char *>(ivs[i].iov_base));
    std::cout << b;
  }
  close(fd);
  return 0;
}

int main(int argc, char *argv[]) {
  if (argc < 2) {
    fprintf(stderr, "Usage: %s <filename1> [<filename2> ...]\n", argv[0]);
    return 1;
  }

  for (int i = 1; i < argc; ++i) {
    cat(argv[i]);
  }
}