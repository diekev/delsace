#include <fcntl.h>      // for open, O_RDONLY
#include <stdio.h>      // for printf, perror
#include <stdlib.h>     // for exit
#include <string.h>     // for memset
#include <sys/stat.h>   // for stat
#include <unistd.h>     // for read

void match_pattern(char *argv[])
{
	int fd;
	if ((fd = open(argv[2], O_RDONLY)) == -1) {
		return;
	}

	char temp;
	char line[100];
	int j = 0;

	while (read(fd, &temp, sizeof(char)) != 0) {
		if (temp != '\n') {
			line[j++] = temp;
			continue;
		}

		if (strstr(line, argv[1]) != NULL)
			printf("%s\n", line);

		memset(line, 0, sizeof(line));
		j = 0;
	}
}

int main(int argc, char *argv[])
{
	if (argc < 3) {
		printf("Usage: %s pattern file\n", argv[0]);
		exit(1);
	}

	struct stat stt;
	if (stat(argv[2], &stt) != 0) {
		perror("stat()");
		exit(1);
	}

	match_pattern(argv);

	exit(0);
}
