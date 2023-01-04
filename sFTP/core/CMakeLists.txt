add_library(sFTP_Core config_man.cxx utilities.cxx net_man.cxx socket_man.cxx)
target_include_directories(sFTP_Core PUBLIC "../dependencies")