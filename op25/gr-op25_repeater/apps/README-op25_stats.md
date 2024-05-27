The `op25_stats.sh` program is a Bash shell script that processes a log file generated by the `rx.py` program and a "tag" file that has descriptive text identifying individual talkgroups.

The program takes as arguments the logfile and tag file names, for example if run from within the `op25/op25/gr-op25_repeater/apps` directory:

```
./op25_stats.sh stderr.2 montgomery.tsv
```

To generate the log data the script needs, run `rx.py` with the argument `-v 5`.  **Do not** run with `-v 10` on a Raspberry Pi or other slow(er) machine as that amount of output will probably result in log file corruption.

The script generates several output files in the current directory:

- `op25-freqs-used.txt`: A list of the RF channels used by the monitored site
- `op25-cc-used.txt`: A list of the control channels used by the monitored site
- `op25-new-tgids.txt`: A list of the talkgroups seen on the site
- `op25-tgid-frequency.txt`: Talkgroups sorted by how frequently used (from most to least)
- `op25-tagless-frequency.txt`: Talkgroups for which there is no tag file entry, sorted by frequency of use (most to least)
- `op25-tagless-numeric.txt`: Talkgroups for which there is no tag file entry, sorted numerically.  (This is very useful when searching for tags from another list.)
- `op25-activity.txt`: A histogram of system activity, showing the number of calls within each five minute period.  This file can be fed into a graphing program to show activity vs. time.