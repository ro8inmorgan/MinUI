#!/bin/sh
# governor.sh - CPU governor controller for H700
# Usage: governor.sh <mode>
# Modes: auto, performance, powersave

MODE="$1"
[ -z "$MODE" ] && MODE="auto"

set_policy() {
	local policy_path="$1"
	local governor="$2"
	local max_type="$3"  # "second_max", "max", or "mid"
	
	[ -f "$policy_path/scaling_available_frequencies" ] || return 0
	FREQS=$(cat "$policy_path/scaling_available_frequencies" | tr ' ' '\n' | grep -v '^$' | sort -n)
	MIN_FREQ=$(echo "$FREQS" | head -1)
	
	case "$max_type" in
		second_max)
			MAX_FREQ=$(echo "$FREQS" | tail -2 | head -1)
			;;
		max)
			MAX_FREQ=$(echo "$FREQS" | tail -1)
			;;
		mid)
			COUNT=$(echo "$FREQS" | wc -l)
			MID=$(( (COUNT + 1) / 2 ))
			MAX_FREQ=$(echo "$FREQS" | sed -n "${MID}p")
			;;
		*)
			MAX_FREQ=$(echo "$FREQS" | tail -1)
			;;
	esac
	
	echo "$governor" > "$policy_path/scaling_governor" 2>/dev/null || true
	echo "$MIN_FREQ" > "$policy_path/scaling_min_freq"
	echo "$MAX_FREQ" > "$policy_path/scaling_max_freq"
}

case "$MODE" in
	auto)
		set_policy /sys/devices/system/cpu/cpufreq/policy0 "schedutil" "second_max"
		set_policy /sys/devices/system/cpu/cpu0/cpufreq "schedutil" "second_max"
		;;
	performance)
		set_policy /sys/devices/system/cpu/cpufreq/policy0 "performance" "max"
		set_policy /sys/devices/system/cpu/cpu0/cpufreq "performance" "max"
		;;
	powersave)
		set_policy /sys/devices/system/cpu/cpufreq/policy0 "conservative" "mid"
		set_policy /sys/devices/system/cpu/cpu0/cpufreq "conservative" "mid"
		;;
	*)
		echo "governor.sh: unknown mode '$MODE'" >&2
		echo "  Valid modes: auto, performance, powersave" >&2
		exit 1
		;;
esac
