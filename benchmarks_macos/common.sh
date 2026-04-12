#!/bin/bash

file_size() {
    if stat -f%z "$1" >/dev/null 2>&1; then
        stat -f%z "$1"
    else
        stat -c%s "$1"
    fi
}

now_ms() {
    perl -MTime::HiRes=time -e 'printf "%.0f\n", time() * 1000'
}

human_size() {
    awk -v bytes="$1" 'BEGIN {
        split("B KiB MiB GiB TiB PiB", units, " ");
        idx = 1;
        while (bytes >= 1024 && idx < 6) {
            bytes /= 1024;
            idx++;
        }
        if (idx == 1) {
            printf "%d%s", bytes, units[idx];
        } else {
            printf "%.1f%s", bytes, units[idx];
        }
    }'
}

float_lt() {
    awk -v a="$1" -v b="$2" 'BEGIN { exit !(a + 0 < b + 0) }'
}

is_numeric() {
    case "$1" in
        ''|*[!0-9.]*|*.*.*)
            return 1
            ;;
        *)
            return 0
            ;;
    esac
}

kill_process_tree() {
    local signal="$1"
    local pid="$2"
    local children
    local child

    children=$(pgrep -P "$pid" 2>/dev/null || true)
    for child in $children; do
        kill_process_tree "$signal" "$child"
    done

    kill "-$signal" "$pid" 2>/dev/null || true
}

run_with_timeout() {
    local timeout_seconds="$1"
    local command="$2"
    local timeout_flag
    local cmd_pid
    local watcher_pid
    local status

    timeout_flag=$(mktemp "${TMPDIR:-/tmp}/g07_timeout.XXXXXX")
    rm -f "$timeout_flag"

    bash -c "$command" >/dev/null 2>&1 &
    cmd_pid=$!

    (
        sleep "$timeout_seconds"
        if kill -0 "$cmd_pid" 2>/dev/null; then
            : > "$timeout_flag"
            kill_process_tree TERM "$cmd_pid"
            sleep 1
            if kill -0 "$cmd_pid" 2>/dev/null; then
                kill_process_tree KILL "$cmd_pid"
            fi
        fi
    ) &
    watcher_pid=$!

    wait "$cmd_pid" 2>/dev/null
    status=$?

    kill "$watcher_pid" 2>/dev/null || true
    wait "$watcher_pid" 2>/dev/null || true

    if [ -f "$timeout_flag" ]; then
        rm -f "$timeout_flag"
        return 124
    fi

    rm -f "$timeout_flag"
    return "$status"
}
