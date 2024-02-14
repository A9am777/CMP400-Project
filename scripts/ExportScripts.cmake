# Copy relevant scripts to the build directory

set(trueCopyDir "${CMAKE_CURRENT_LIST_DIR}/Exports")
getDirFiles(ExportScripts "${trueCopyDir}" "CMFT_SCRIPT_batch" 1)

logInf(${ExportScripts})
function(exportScripts target)
   # Shorthands
   set(trueOutputDir "$<TARGET_FILE_DIR:${target}>")

   # Compiled shaders
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
endfunction()