#!/bin/bash

echo "Starting crazy test..."

siege -c 60 -k http://localhost:5252/timeout/ &

siege -c 100 -k http://localhost:5252/ &

siege -c 60 -t 1m -f urls.txt &

siege -c 100 -t 2m -f urls.txt &


run_curl_loops() {
    while true; do
        curl -X POST -d "THIS WILL TIMEOUT" http://localhost:5252/upload/ &
        curl -X POST -F 'file=@./cometled.mp4' http://localhost:5252/upload/ &
        curl -X DELETE "http://localhost:5252/delete/?filename=cometled.mp4" &
        curl -X POST -F 'file=@./largefile.iso' http://localhost:5252/upload/ &
        curl -X POST -d "Some test data" http://localhost:5252/test/ &
        sleep 2
    done
}

run_curl_head_loops() {
    while true; do
        curl --head http://localhost:5252/ &
        curl --head http://localhost:5252/uploads/ &
        curl --head http://localhost:5252/list/ &
        curl --head http://localhost:5252/timeout/ &
        curl --head http://localhost:5252/nonexistent/ &
        curl --head http://localhost:5252/facts/ &
        curl --head http://localhost:5252/api/status &

        sleep 2
    done
}


run_ffuf_tests() {
    while true; do
        ffuf -w localhost_wordlist.txt -u http://localhost:5252/FUZZ -mc 200 -o ffuf_localhost_results.txt &
        ffuf -w legit_wordlist.txt -u http://localhost:5252/FUZZ -mc 200 -o ffuf_legit_results.txt &
        ffuf -w random_wordlist.txt -u http://localhost:5252/FUZZ -mc 200 -o ffuf_random_results.txt &
        ffuf -w random_wordlist.txt -u http://localhost:5252/test.php?FUZZ=value -mc 200 -o ffuf_param_results.txt &
        sleep 5
    done
}


run_curl_loops &
run_curl_head_loops &
run_ffuf_tests &

wait

echo "Crazy test completed!"
