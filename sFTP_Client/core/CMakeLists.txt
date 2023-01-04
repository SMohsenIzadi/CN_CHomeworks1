add_library(sFTP_Client_Core utilities.cxx socket_man.cxx)
target_include_directories(sFTP_Client_Core PUBLIC "../dependencies")