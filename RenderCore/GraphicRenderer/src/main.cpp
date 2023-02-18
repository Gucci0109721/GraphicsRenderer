#include "Application.h"

int wmain(int argc,wchar_t** argv,wchar_t** evnp)
{
	Application app(960, 540);
	app.Run();

	return 0;
}