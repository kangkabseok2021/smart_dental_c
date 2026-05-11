find_package(Qt6 REQUIRED COMPONENTS Test Core Sql)

qt_add_executable(tst_HardwareHandler
    tst_HardwareHandler.cpp
    ../src/HardwareHandler.cpp
)
target_include_directories(tst_HardwareHandler PRIVATE ../src)
target_link_libraries(tst_HardwareHandler PRIVATE Qt6::Test Qt6::Core)
add_test(NAME tst_HardwareHandler COMMAND tst_HardwareHandler)

qt_add_executable(tst_StateManager
    tst_StateManager.cpp
    ../src/StateManager.cpp
    ../src/DatabaseManager.cpp
)
target_include_directories(tst_StateManager PRIVATE ../src)
target_link_libraries(tst_StateManager PRIVATE Qt6::Test Qt6::Core Qt6::Sql)
add_test(NAME tst_StateManager COMMAND tst_StateManager)
