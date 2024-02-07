#include "Conversions.h"

double Conversions::db2lin(double db)
{
    return pow(10, db / 10);
}

double Conversions::lin2db(double lin)
{
    return 10 * log10(lin);
}