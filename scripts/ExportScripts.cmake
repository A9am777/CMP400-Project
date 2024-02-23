# Copy relevant scripts to the build directory

set(trueCopyDir "${CMAKE_CURRENT_LIST_DIR}/Exports")
getDirFiles(ExportBatScripts "${trueCopyDir}" "CMFT_SCRIPT_batch" 1)
getDirFiles(ExportPyScripts "${trueCopyDir}" "CMFT_SCRIPT_python" 1)

logInf("${ExportBatScripts}")
logInf("${ExportPyScripts}")
function(exportScripts target)
   # Shorthands
   set(trueOutputDir "$<TARGET_FILE_DIR:${target}>")

   set(ExportScripts ${ExportBatScripts};${ExportPyScripts})

   foreach(script ${ExportScripts})
      getPureFilename(${script} scriptFilename)
      string(REPLACE "${trueCopyDir}" "" scriptRelFile "${script}")

      set_property(
            DIRECTORY
            APPEND
            PROPERTY CMAKE_CONFIGURE_DEPENDS "${trueCopyDir}/${scriptRelFile}")
      add_custom_command(TARGET ${target}
         POST_BUILD
         COMMAND ${CMAKE_COMMAND} 
         ARGS -E copy_if_different "${trueCopyDir}/${scriptRelFile}" "${trueOutputDir}/${scriptRelFile}"
         MAIN_DEPENDENCY ${script}
         COMMENT "EXPORTSCRIPTS COPY ${script}"
         WORKING_DIRECTORY ${trueCopyDir}
         VERBATIM)
   endforeach()

   set(PROFILER_PATH "${CMAKE_CURRENT_LIST_DIR}/../Tracy-0.10" CACHE STRING "The path to the profiler applications")
   add_custom_command(TARGET ${target}
         POST_BUILD
         COMMAND IF NOT EXIST "${trueOutputDir}/Profiler " mklink /J "${trueOutputDir}/Profiler " "${PROFILER_PATH} "
         MAIN_DEPENDENCY ${script}
         COMMENT "EXPORTSCRIPTS COPY ${script}"
         WORKING_DIRECTORY ${trueCopyDir}
         VERBATIM)
   set(DXTEX_PATH "${CMAKE_CURRENT_LIST_DIR}/../DirectXTex" CACHE STRING "The path to the texture processing applications")
   add_custom_command(TARGET ${target}
         POST_BUILD
         COMMAND IF NOT EXIST "${trueOutputDir}/DXT " mklink /J "${trueOutputDir}/DXT " "${DXTEX_PATH} "
         MAIN_DEPENDENCY ${script}
         COMMENT "EXPORTSCRIPTS COPY ${script}"
         WORKING_DIRECTORY ${trueCopyDir}
         VERBATIM)
endfunction()