#include "types.h"
#include "user.h"
#include "jsmn.h"

/* error codes */
int FAILED_TO_PARSE_JSON = -1;

int
strncmp_const(const char *p, const char *q, uint n)
{
  while(n > 0 && *p && *p == *q)
    n--, p++, q++;
  if(n == 0)
    return 0;
  return (uchar)*p - (uchar)*q;
}

uint strlen_const(const char *s)
{
	int n;

	for (n = 0; s[n]; n++)
		;
	return n;
}

int jsoneq(const char *json, jsmntok_t *tok, const char *s) {
	if (tok->type == JSMN_STRING && (int) strlen_const(s) == tok->end - tok->start &&
			strncmp_const(json + tok->start, s, tok->end - tok->start) == 0) {
		return 0;
	}
	return -1;
}

char * substring(char * target, char * source, int start, int end) {
	char * ret;
	ret = target;
	int i;
	for (i=start; i <= end; i++) {
		target[i-start] = source[i];
	}
	target[end-start] = '\0';
	return ret;
}

int parse_spec(char * specfile, char * init, char * fs, int * nproc) {
	int fd = open(specfile,0);
	char buf[512];
	
	int i = 0;
	while((i = read(fd, buf, sizeof(buf))) > 0);
	close(fd);
	// printf(1,"spec: %s\n",buf);

	// parse file
	jsmn_parser p;
	jsmntok_t t[128];

	jsmn_init(&p);
  	int r = jsmn_parse(&p, buf, strlen(buf), t,
                 sizeof(t) / sizeof(t[0]));

	if (r < 0) {
		printf(1,"Failed to parse JSON: %d\n", r);
		return FAILED_TO_PARSE_JSON;
  	}

	/* Assume the top-level element is an object */
	if (r < 1 || t[0].type != JSMN_OBJECT) {
		printf(1,"Object expected\n");
		return 1;
	}

	for (i = 1; i < r; i++) {
		if (jsoneq(buf, &t[i], "init") == 0) {
			int len = t[i+1].end-t[i+1].start;
			char * source = buf + t[i+1].start;
			substring(init, source, 0, len);
			i++;
		} else if (jsoneq(buf, &t[i], "fs") == 0) {
			int len = t[i+1].end-t[i+1].start;
			char * source = buf + t[i+1].start;
			substring(fs, source, 0, len);
			i++;
		} else if (jsoneq(buf, &t[i], "nproc") == 0) {
			int len = t[i+1].end-t[i+1].start;
			char * source = buf + t[i+1].start;
			char nproc_str[5];
			substring(nproc_str, source, 0, len);
			*nproc = atoi(nproc_str);
			i++;
		}
	}
	return 0;
}

int cp_func(char * src, char * dest){
	char buf[512];
	int fd, dfd, r, w = -1;

	if ((fd = open(src, O_RDONLY)) < 0) {
		printf(2, "cp: cannot open source %s\n", src);
		exit();
	}
	if ((dfd = open(dest, O_CREATE|O_WRONLY)) < 0) {
		printf(2, "cp: cannot open destination %s\n", dest);
		exit();
	}
	
	while ((r = read(fd, buf, sizeof(buf))) > 0) {
		w = write(dfd, buf, r);
		if (w != r || w < 0) 
		break;
	}
	if (r < 0 || w < 0)
		printf(2, "cp: error copying %s to %s\n", src, dest);

	close(fd);
	close(dfd);

	return 0;
}

int init_dir(char * fs) {
	// create directory
	if(mkdir(fs) != 0) {
		printf(1, "Creating container failed. Container taken.\n");
		return -1;
	}

	// copy sh, ls, cat to the new directory
	char *files[10] = {"ls","cat","sh","cp","mkdir","ps","pscon","hello","makeproc","text1.txt"};
	int nfiles = 10;
	int i;
	for (i=0; i < nfiles; i++) {
		char dest[64];
		strcpy(dest, fs);
		strcat(dest, "/");
		strcat(dest, files[i]);
		strcat(dest, "\0");
		// printf(1,"src: %s\ndes: %s\n", files[i],dest);
		cp_func(files[i],dest);
	}

	return 0;
}