package main

import (
	"fmt"
	"log"
	"net/http"
	"runtime"
	"strconv"
	"sync/atomic"
)

var (
	bootCount  uint64
	ipAddr     = "192.168.0.1"
	board      = "Go Test server"
	did        = ""
	ipStatic   = false
	subnetMask = ""
	gateway    = ""
)

func main() {
	// Initialize boot count
	bootCount = 0

	// Handle requests
	http.HandleFunc("/", serveIndex)
	http.HandleFunc("/board", boardHandler)
	http.HandleFunc("/bootcount", bootCountHandler)
	http.HandleFunc("/cpu-usage", cpuUsageHandler)
	http.HandleFunc("/settings", settingsHandler)
	http.HandleFunc("/did", didHandler)                // New endpoint for FID
	http.HandleFunc("/ipaddr", ipAddrHandler)          // New endpoint for IP address
	http.HandleFunc("/ipstatic", ipStaticHandler)      // New endpoint for IP address
	http.HandleFunc("/subnet-mask", subnetMaskHandler) // New endpoint for IP address
	http.HandleFunc("/gateway", gatewayHandler)        // New endpoint for IP address

	// Start the server
	log.Println("Starting server on :8080...")
	err := http.ListenAndServe(":8080", nil)
	if err != nil {
		log.Fatalf("Could not start server: %s\n", err.Error())
	}
}

func serveIndex(w http.ResponseWriter, r *http.Request) {
	// Serve the index.html file for the root URL
	http.ServeFile(w, r, "../src/index.html")
}

func boardHandler(w http.ResponseWriter, r *http.Request) {
	w.Header().Set("Content-Type", "text/plain")
	_, _ = w.Write([]byte(board))
}

func bootCountHandler(w http.ResponseWriter, r *http.Request) {
	count := atomic.AddUint64(&bootCount, 1)

	w.Header().Set("Content-Type", "text/plain")
	_, _ = w.Write([]byte(strconv.FormatUint(count, 10)))
}

func cpuUsageHandler(w http.ResponseWriter, r *http.Request) {
	cpuUsage := float64(runtime.NumGoroutine()) / float64(runtime.NumCPU()) * 100

	w.Header().Set("Content-Type", "text/plain")
	_, _ = w.Write([]byte(fmt.Sprintf(`%.2f`, cpuUsage)))
}

func settingsHandler(w http.ResponseWriter, r *http.Request) {
	if r.Method != http.MethodPost {
		http.Error(w, "Invalid request method", http.StatusMethodNotAllowed)
		return
	}

	err := r.ParseForm()
	if err != nil {
		http.Error(w, "Failed to parse form data", http.StatusBadRequest)
		_, _ = w.Write([]byte("failed to parse"))
		return
	}

	did = r.FormValue("did")
	ipAddr = r.FormValue("ipaddr")
	subnetMask = r.FormValue("subnet-mask")
	gateway = r.FormValue("gateway")
	ipStaticS := r.FormValue("ipstatic")
	if ipStaticS == "true" {
		ipStatic = true
	} else {
		ipStatic = false
	}

	log.Printf("Received form data - did: %s, ipStaticS: %s\n", did, ipStaticS)

	w.Header().Set("Content-Type", "application/json")
	_, _ = w.Write([]byte("Settings saved."))
}

func didHandler(w http.ResponseWriter, r *http.Request) {
	w.Header().Set("Content-Type", "text/plain")
	_, _ = w.Write([]byte(did))
}

func ipAddrHandler(w http.ResponseWriter, r *http.Request) {
	w.Header().Set("Content-Type", "text/plain")
	_, _ = w.Write([]byte(ipAddr))
}

func ipStaticHandler(w http.ResponseWriter, r *http.Request) {
	// Respond with the current IP address as plain text
	w.Header().Set("Content-Type", "text/plain")
	if ipStatic {
		_, _ = fmt.Fprintln(w, "true")
	} else {
		_, _ = fmt.Fprintln(w, "false")
	}
}

func subnetMaskHandler(w http.ResponseWriter, r *http.Request) {
	w.Header().Set("Content-Type", "text/plain")
	_, _ = w.Write([]byte(subnetMask))
}

func gatewayHandler(w http.ResponseWriter, r *http.Request) {
	w.Header().Set("Content-Type", "text/plain")
	_, _ = w.Write([]byte(gateway))
}
