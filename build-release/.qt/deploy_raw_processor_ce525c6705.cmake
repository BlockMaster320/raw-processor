include("C:/Storage/FIT/BP/raw-processor/build-release/.qt/QtDeploySupport.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/raw-processor-plugins.cmake" OPTIONAL)
set(__QT_DEPLOY_I18N_CATALOGS "qtbase")

qt6_deploy_runtime_dependencies(
    EXECUTABLE "C:/Storage/FIT/BP/raw-processor/build-release/raw-processor.exe"
    GENERATE_QT_CONF
)
