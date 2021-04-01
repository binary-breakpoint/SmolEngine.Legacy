#include "Main.h"

#include <string>

#include "GraphicsContext.h"
#include "Common/EventHandler.h"

using namespace Frostium;

int main(int argc, char** argv)
{
	Ref<Frostium::EventHandler> eventH = std::make_shared<EventHandler>();
	GraphicsContextInitInfo info = {};
	WindowCreateInfo windoInfo = {};

	windoInfo.EventHandler = eventH;
	info.WindowInfo = &windoInfo;

	GraphicsContext::Init(&info);
}