#include "DisplayManager.h"
#include "MonoDisplayManager.h"

DisplayManager* DisplayManager::create() {
  // Create an instance of the display manager
    
  
  if (std::string(DISPLAY_DRIVER) == "213Mono") {
        return new MonoDisplayManager(false);
    }
    if (std::string(DISPLAY_DRIVER) == "213TriColor") {
        return new MonoDisplayManager(true);
    }

    return nullptr;  // Return null if no valid display driver is found
}
