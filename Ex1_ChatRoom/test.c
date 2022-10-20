#include <stdio.h>
#include "config.h"

int main() {
   char msg[PREFIX_LENGTH + BUFFER] = "Good morning!";

   WrapMessage(TYPE_SEND, msg, (time_t)NULL);

   printf("%s\n", msg);
}