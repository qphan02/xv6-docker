#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BUFFER_SIZE 256

int main(int argc, char* argv[]) {
  int pipe_fd[2]; // Array to store the file descriptors for the pipe

  // Create the pipe
  if (pipe(pipe_fd) == -1) {
    perror("Failed to create pipe");
    return 1;
  }

  // Fork the process to create the child process
  pid_t pid = fork();

  if (pid == -1) {
    perror("Failed to fork process");
    return 1;
  }

  // In the child process
  if (pid == 0) {
    // Close the write end of the pipe, since the child process
    // only needs to read from the pipe
    close(pipe_fd[1]);

    // Read data from the pipe
    char buffer[BUFFER_SIZE];
    ssize_t bytes_read = read(pipe_fd[0], buffer, BUFFER_SIZE);
    if (bytes_read == -1) {
      perror("Failed to read from pipe");
      return 1;
    }

    printf("Child process received: %s\n", buffer);

    // Close the read end of the pipe
    close(pipe_fd[0]);
  }
  // In the parent process
  else {
    // Close the read end of the pipe, since the parent process
    // only needs to write to the pipe
    close(pipe_fd[0]);

    // Write some data to the pipe
    const char* message = "Hello from the parent process!";
    ssize_t bytes_written = write(pipe_fd[1], message, strlen(message));
    if (bytes_written == -1) {
      perror("Failed to write to pipe");
      return 1;
    }

    // Close the write end of the pipe
    close(pipe_fd[1]);
  }

  return 0;
}
