# Copyright (c) 2026 Nordic Semiconductor

# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

#!/usr/bin/env bash
mkdir -p logs/logs_recycle_bin
mv -v logs/*.log logs/logs_recycle_bin/
mv -v logs/*.txt logs/logs_recycle_bin/
mv -v logs/logs_recycle_bin/example.log logs/