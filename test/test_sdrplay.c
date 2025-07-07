#include <stdio.h>
#include <sdrplay_api.h>

int main() {
    float ver;
    sdrplay_api_ErrT err;
    
    err = sdrplay_api_Open();
    if(err != sdrplay_api_Success) {
        printf("Failed to open API\n");
        return -1;
    }
    
    err = sdrplay_api_ApiVersion(&ver);
    printf("API version: %.2f\n", ver);
    
    sdrplay_api_Close();
    return 0;
}
