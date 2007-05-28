#include <stdio.h>
#include <unistd.h>

int main()
{
  int status;
  char buffer[100];
  while(1) {
    status = read(0, buffer, sizeof(buffer));
    if (status <= 0) {
      return 0;
    }
    status = write(1, buffer, status);
    if (status < 0) {
      perror("mycat write");
      return 1;
    }

    float value = status;
    status = write(3, &value, sizeof(value));
  }

  return 0;
}

