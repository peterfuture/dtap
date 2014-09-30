#include "dtap.h"

int bundle_test();

static void version_info()
{
    dtap_print("|===================================| \n"); 
    dtap_print("| version: dtap v1.0 \n"); 
    dtap_print("| build time "__DATE__ __TIME__" \n"); 
    dtap_print("|===================================| \n"); 
}

static void usage()
{
    dtap_print("|===================================| \n"); 
    dtap_print("| usage \n"); 
    dtap_print("| dtap in_file outfile \n"); 
    dtap_print("|===================================| \n"); 
}

static int feature_select(int *code)
{
    dtap_print("|===================================| \n"); 
    dtap_print("| select test item \n"); 
    dtap_print("| 1 EQ TEST \n"); 
    dtap_print("| 2 BUNDLE TEST \n"); 
    dtap_print("| 3 REVERB TEST \n"); 
    dtap_print("| 9 QUIT TEST \n"); 
    dtap_print("|===================================| \n");

    return scanf("%d]", code);
}

int main(int argc, char ** argv)
{
    version_info();

    if(argc < 3)
    {
        usage();
        return 0;
    }

    char *in = argv[1];
    char *out = argv[2];

    int test_code = -1;
    int ret = feature_select(&test_code);
    if(ret <= 0)
        return 0;

    while(test_code != 9)
    {
        if(test_code == 1)
            bundle_test(in, out);
        ret = feature_select(&test_code);
        if(ret <= 0)
            return 0;
    }
    return 0;
}
