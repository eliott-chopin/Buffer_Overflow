all:
	gcc vulnerable.c -o vuln -fno-stack-protector -z execstack -no-pie

clean:
	rm -f vuln
