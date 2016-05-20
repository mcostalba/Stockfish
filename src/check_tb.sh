rm ./log_probe_wdl.bin
rm ./log_root_probe.txt
./stockfish < sbench > /dev/null
diff log_probe_wdl.bin log_probe_wdl_ms.bin
diff log_root_probe.txt log_root_probe_ms.txt
