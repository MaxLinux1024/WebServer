#include <queue>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <cstdlib>
#include <iostream>
#include <vector>
#include <unistd.h>

void WebServerSegerror( int nsig )
{
	//DA_LOGDBG("%s got segment error !", "sdkdevds");
	//CSddsServer::SetTskState( );
}
void WebServerPipeerror( int nsig )
{
	//DA_LOGDBG("%s got pipe error !", "sdkdevds");
}

void WebServerTerminate( int nsig )
{
	//DA_LOGDBG("%s got User1Signal  !", "sdkdevds");
	//CSddsServer::SetTskState( );
	return ;
}


void HandleSigpipe()
{
	struct sigaction sa;
	sa.sa_flags = 0;

    sigemptyset(&sa.sa_mask);	
	sigaddset(&sa.sa_mask, SIGPIPE);    	
	sigaddset(&sa.sa_mask, SIGTERM);	
	sigaddset(&sa.sa_mask, SIGINT);	 

	sa.sa_handler = WebServerPipeerror;   
	sigaction(SIGPIPE, &sa, NULL);    	
	sa.sa_handler = WebServerTerminate;    
	sigaction(SIGTERM, &sa, NULL);		
	sa.sa_handler = WebServerTerminate;    
	sigaction(SIGINT, &sa, NULL);
}

int main()
{
	
	return 0;
}

