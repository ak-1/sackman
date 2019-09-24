
error() {
    echo "Error: $msg" >/dev/stderr
    exit 1
}

info() {
    echo "$@" > /dev/stderr
}

call() {
    info "> $@"
    "$@"
}
