#include <stdio.h>
#include <sdrplay_api.h>

int main() {
    float ver = 0.0;
    sdrplay_api_Open();
    sdrplay_api_ApiVersion(&ver);
    printf("SDRplay API Version: %f\n", ver);
    sdrplay_api_Close();
    return 0;
}
