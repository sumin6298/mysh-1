#include "signal_handlers.h"
#include <stdio.h>
#include<unistd.h>
#include<signal.h>

void catch_sigint(int signalNo)
{
 signal(signalNo, SIG_IGN);
}

void catch_sigtstp(int signalNo)
{
 signal(signalNo, SIG_IGN);
}
