#include "dtap.h"
#include "stdio.h"

int bundle_test();

static void usage()
{
    dtap_print("|=============================| \n"); 
    dtap_print("| version: dtap v1.0 \n"); 
    dtap_print("| dtap in_file outfile \n"); 
    dtap_print("|=============================| \n"); 
}

int main(int argc, char ** argv)
{
    usage();
    bundle_test();
    return 0;
}
