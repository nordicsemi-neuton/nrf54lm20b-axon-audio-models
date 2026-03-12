:: Copyright (c) 2026 Nordic Semiconductor
::
:: SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

@echo off
if not exist logs\logs_recycle_bin\ mkdir logs\logs_recycle_bin\
move /Y logs\*.log logs\logs_recycle_bin\
move /Y logs\*.txt logs\logs_recycle_bin\
move /Y logs\logs_recycle_bin\example.log logs\