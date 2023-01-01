#include "config_man.hxx"

int main() {

    if(!load_configs())
    {
        //Error in reading config file
        return -2;
    }

    return 0;
}