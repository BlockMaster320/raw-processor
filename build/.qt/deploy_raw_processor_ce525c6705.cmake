include("C:/Storage/FIT/BP/raw-processor/build/.qt/QtDeploySupport.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/raw-processor-plugins.cmake" OPTIONAL)
set(__QT_DEPLOY_I18N_CATALOGS "qtbase")

qt6_deploy_runtime_dependencies(
    EXECUTABLE "C:/Storage/FIT/BP/raw-processor/build/raw-processor.exe"
    GENERATE_QT_CONF
)
