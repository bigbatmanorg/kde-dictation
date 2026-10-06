function(kdict_add_test name source)
    add_executable(${name} ${source})
    target_link_libraries(${name} PRIVATE kde_dictation_core Qt6::Test)
    add_test(NAME ${name} COMMAND ${name})
endfunction()

kdict_add_test(test_state test_state.cpp)
kdict_add_test(test_config test_config.cpp)
kdict_add_test(test_wav test_wav.cpp)
kdict_add_test(test_realtime_parser test_realtime_parser.cpp)
kdict_add_test(test_fallback test_fallback.cpp)
kdict_add_test(test_url test_url.cpp)

kdict_add_test(test_live_reconciliation test_live_reconciliation.cpp)
