# Simple FTL Simulator

A small project I practiced during my internship at Phison.

---

## 📁 Folder Structure

```bash

Simple-FTL-Simulator/
├── Config File/                # Configuration files for SSD settings
├── pl-1GC 1log code/           # Main simulator source code and shell script
├── result/                     # Output files after running simulation (not pushed)
└── trace/                      # Large trace files (not pushed)
    ├── rawfile952G_simple_LUN0.txt
    ├── rawfile952G_simple_LUN1.txt
    └── ...
```

> Only `Config File/` and `pl-1GC 1log code/` are included in the repo.  
> The `result/` folder is generated after execution.  
> The `trace/` folder is too large to upload to GitHub.

---

## 🚀 How to Run

1. **Download the traces**  
   Refer to [Trace Download Instructions](#-how-to-download-traces) and place them under `trace/`.

2. **Create Config**  
   Create a config file like those in `Config File/my_config/` to specify your SSD setup:  
   e.g., total size, page size, block size, endurance of each block.  
   (The filename can be arbitrary.)

3. **Edit test.sh Parameters**  
   Modify the `params` variable in `pl-1GC 1log code/test.sh` to match the config filename you created above.

4. **Run the Simulation**  
   ```bash
   cd "pl-1GC 1log code/"
   make          # if you’ve changed the code
   ./test.sh
   ```

---

## 📥 How to Download Traces

We use **Systor '17** traces.

1. Visit: [https://iotta.snia.org/traces/block-io](https://iotta.snia.org/traces/block-io)  
   → Find the **Systor '17 Traces** section  
   → Use “Choose an action” to download

2. After extraction, you will get a `Systor_17_Traces/` folder with many compressed files.

3. Place `get_raw.sh` inside that folder and run it:
   ```bash
   ./get_raw.sh
   ```
   This will:
   - Unzip files into `rawfile/`
   - Merge sub-traces by LUN
   - Output combined CSVs like: `LUN{0,1,2,3,4,6}.csv`

4. Place `raw2trace_RW.py` in the same folder and run:
   ```bash
   python3 raw2trace_RW.py
   ```
   This converts each LUN CSV into a readable trace file in the format:
   ```
   <i(LUN number), option (R/W), sector offset, sector length>
   e.g., <0 W 32116 1.0>
   ```

s
5. Move the following files into your `trace/` folder:
   ```
   rawfile952G_simple_LUN{0,1,2,3,4,6}.txt
   ```

---

## 💡 Notes

- Make sure your `.txt` trace files are placed under the `trace/` folder before running the simulator.
- Re-run `make` if you've modified any `.c` source files.

---

## 📜 License

This project is for educational and demonstration purposes.
