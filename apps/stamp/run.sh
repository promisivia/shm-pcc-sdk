#!/bin/bash

# Default values
APP="all"
THREADS=4

# Help message
function show_help {
    echo "Usage: $0 [options] [app_args...]"
    echo "Options:"
    echo "  -a, --app APP       Specify application to run (bayes|genome|intruder|kmeans|labyrinth|ssca2|vacation|yada|all)"
    echo "  -t, --threads N     Number of threads (default: 4)"
    echo "  -h, --help          Show this help message"
    echo ""
    echo "Additional arguments will be passed directly to the application."
    exit 1
}

# Parse command line arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        -a|--app)
            APP="$2"
            shift 2
            ;;
        -t|--threads)
            THREADS="$2"
            shift 2
            ;;
        -h|--help)
            show_help
            ;;
        *)
            # Store remaining arguments for the application
            APP_ARGS+=("$1")
            shift
            ;;
    esac
done

# Function to run an application
function run_app {
    local app=$1
    local threads=$2
    shift 2
    local app_args=("$@")
    
    echo "Running $app with $threads threads"
    echo "Additional arguments: ${app_args[@]}"
    
    case $app in
        "bayes")
            ./build/bayes/bayes -t $threads "${app_args[@]}"
            ;;
        "genome")
            ./build/genome/genome -t $threads "${app_args[@]}"
            ;;
        "intruder")
            ./build/intruder/intruder -t $threads "${app_args[@]}"
            ;;
        "kmeans")
            ./build/kmeans/kmeans -p $threads "${app_args[@]}"
            ;;
        "labyrinth")
            ./build/labyrinth/labyrinth -t $threads "${app_args[@]}"
            ;;
        "ssca2")
            ./build/ssca2/ssca2 -t $threads "${app_args[@]}"
            ;;
        "vacation")
            ./build/vacation/vacation -t $threads "${app_args[@]}"
            ;;
        "yada")
            ./build/yada/yada -t $threads "${app_args[@]}"
            ;;
        *)
            echo "Unknown application: $app"
            exit 1
            ;;
    esac
}

# Main execution
echo "Starting STAMP benchmark suite"
echo "============================="

if [ "$APP" = "all" ]; then
    # Run all applications
    for app in bayes genome intruder kmeans labyrinth ssca2 vacation yada; do
        echo "============================="
        run_app $app $THREADS "${APP_ARGS[@]}"
    done
else
    # Run specified application
    run_app $APP $THREADS "${APP_ARGS[@]}"
fi

echo "============================="
echo "Benchmark suite completed"
