# revision.cmake - for board: nv_n1x_yukon
# BOARD_REVISION 是使用者輸入的版本 (via west build -b nv_n1x_yukon@b00)

# 所有合法版本
set(SUPPORTED_REVISIONS b00 b01)

# 驗證 revision 是否存在
list(FIND SUPPORTED_REVISIONS ${BOARD_REVISION} index)
if(index EQUAL -1)
  message(FATAL_ERROR "Unsupported board revision: ${BOARD_REVISION}")
endif()

# 如果要套用原始版本，這樣寫即可
set(ACTIVE_BOARD_REVISION ${BOARD_REVISION})

# 也可以改寫，如有 alias 情況：
# if(${BOARD_REVISION} STREQUAL "rev1")
#     set(ACTIVE_BOARD_REVISION "b00")
# elseif(${BOARD_REVISION} STREQUAL "rev2")
#     set(ACTIVE_BOARD_REVISION "b01")
# endif()
