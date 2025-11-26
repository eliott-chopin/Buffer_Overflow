#include <stdio.h>
#include <string.h>

void vulnerable_function(char *input) {
    char buffer[32];
    printf("[*] Buffer size = 32 bytes\n");

    // Vulnérable : copie non contrôlée
    strcpy(buffer, input);

    printf("[*] Buffer content: %s\n", buffer);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s <payload>\n", argv[0]);
        return 1;
    }

    vulnerable_function(argv[1]);
    return 0;
}
