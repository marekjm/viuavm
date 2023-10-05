#include <errno.h>
#include <stdint.h>
#include <string.h>

int main()
{
    strerrorname_np(errno);
    return 0;
}
