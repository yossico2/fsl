"""
Integration tests for FSL using Python sockets.
Simulates GSL (UDP client) and two apps (UDS clients/servers).
"""

import os
import math
import time
import struct
import socket
import threading

# GslFslHeader: opcode (uint16), sensor_id (uint16), length (uint32), seq_id (uint32)
GSL_FSL_HEADER_SIZE = struct.calcsize("<HHII")

UDP_MTU = 65000


def send_udp_to_fsl(opcode, payload, udp_ip, udp_port, sensor_id=0):
    """Simulate GSL: Send UDP packet to FSL with header and payload."""
    # GslFslHeader: opcode (uint16), sensor_id (uint16), length (uint32), seq_id (uint32)
    msg_seq_id = 1
    header = struct.pack("<HHII", opcode, sensor_id, len(payload), msg_seq_id)
    packet = header + payload.encode()
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    s.sendto(packet, (udp_ip, udp_port))
    s.close()


def init_uds_path(uds_path):
    """Ensure UDS path's parent directory exists."""
    instance = os.environ.get("STATEFULSET_INDEX")
    if instance:
        parent = os.path.dirname(uds_path)
        if not os.path.exists(parent):
            os.makedirs(parent, exist_ok=True)


def send_uds_to_fsl(uds_path, payload, sock):
    """Simulate app: Send payload (bytes) to FSL UDS server socket. Requires a socket. Retries on WOULD_BLOCK."""
    import errno

    max_retries = 10
    for attempt in range(max_retries):
        try:
            sock.sendto(payload, uds_path)
            return
        except OSError as e:
            if e.errno in (errno.EAGAIN, errno.EWOULDBLOCK):
                if attempt < max_retries - 1:
                    time.sleep(0.1)
                    continue
                else:
                    print(
                        f"[ERROR] send_uds_to_fsl: Buffer full after {max_retries} retries, giving up."
                    )
                    raise
            else:
                raise


def uds_receiver_thread(uds_path, result_container):
    """Simulate app: Receive from FSL UDS client socket."""
    init_uds_path(uds_path)
    if os.path.exists(uds_path):
        os.unlink(uds_path)

    timeout = 2
    s = socket.socket(socket.AF_UNIX, socket.SOCK_DGRAM)
    s.bind(uds_path)
    s.settimeout(timeout)
    try:
        data, _ = s.recvfrom(UDP_MTU)
        result_container.append(data)
    except socket.timeout as error:
        print(f"Socket timeout error: {error}")
    finally:
        s.close()
        os.unlink(uds_path)


def test_ul_udp_to_uds():
    """Test: GSL sends UDP, FSL routes to correct UDS client."""
    fsl_udp_ip = "127.0.0.1"
    fsl_udp_port = 9910
    instance = int(os.environ.get("STATEFULSET_INDEX", "-1"))
    prefix = f"/tmp/sensor-{instance}/" if instance >= 0 else "/tmp/"
    app_uds_clients = [
        prefix + "FSW_UL",
        prefix + "UL_PLMG",
        prefix + "UL_EL",
        # Add more uplink UDS client paths as needed
    ]
    for i, uds_client_path in enumerate(app_uds_clients, start=1):
        opcode = i  # Use different opcode for each app if needed
        payload = f"hello_from_gsl_{i}"
        result = []
        t = threading.Thread(target=uds_receiver_thread, args=(uds_client_path, result))
        t.start()
        time.sleep(0.5)  # Give receiver time to bind
        print(f"Uplink: sending UDP to FSL for {uds_client_path}...")
        send_udp_to_fsl(opcode, payload, fsl_udp_ip, fsl_udp_port)
        t.join()
        received = result[0] if result else None
        assert (
            received is not None
        ), f"No data received on UDS client socket {uds_client_path}"
        print(f"Uplink: received on {uds_client_path}:", received)


def test_dl_uds_to_udp():
    """Test: App sends to UDS server, FSL routes to UDP (GSL)."""
    gcom_udp_ip = "127.0.0.1"
    gcom_udp_port = 9010
    instance = int(os.environ.get("STATEFULSET_INDEX", "-1"))
    prefix = f"/tmp/sensor-{instance}/" if instance >= 0 else "/tmp/"
    app_uds_paths = [
        prefix + "FSW_HIGH_DL",
        prefix + "DL_PLMG_H",
        prefix + "DL_EL_H",
        # Add more app UDS paths as needed
    ]

    uds_sock = socket.socket(socket.AF_UNIX, socket.SOCK_DGRAM)
    try:
        for i, uds_server_path in enumerate(app_uds_paths, start=1):
            payload = f"hello from app{i}".encode()

            # Start UDP receiver (GSL)
            s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
            s.bind((gcom_udp_ip, gcom_udp_port))
            s.settimeout(2)
            print(f"Downlink: sending message to UDS server {uds_server_path}...")

            # Compose message: FSW sends only payload, PLMG/EL send plmg_fcom_header + payload
            if "PLMG" in uds_server_path or "EL" in uds_server_path:
                # plmg_fcom_header: opcode (uint8), error (uint8), seq_id (uint16), length (uint16)
                opcode = 42  # test opcode
                error = 0
                seq_id = 1
                length = len(payload)
                header = struct.pack("<BBHH", opcode, error, seq_id, length)
                msg = header + payload
            else:
                msg = payload

            init_uds_path(uds_server_path)
            send_uds_to_fsl(uds_server_path, msg, uds_sock)
            print("Downlink: receiving message from UDP...")

            try:
                data, addr = s.recvfrom(UDP_MTU)
                print("Downlink: received message from UDP:", data)
                assert data is not None, f"No UDP data received for {uds_server_path}"
                # Parse GslFslHeader: opcode (uint16), sensor_id (uint16), length (uint32), seq_id (uint32), all little-endian
                if data and len(data) >= GSL_FSL_HEADER_SIZE:
                    # GslFslHeader: opcode (2 bytes), sensor_id (2 bytes), length (4 bytes), seq_id (4 bytes)
                    opcode, sensor_id, length, seq_id = struct.unpack(
                        "<HHII", data[:GSL_FSL_HEADER_SIZE]
                    )
                    print(
                        f"GslFslHeader: opcode={opcode}, sensor_id={sensor_id}, length={length}, seq_id={seq_id}"
                    )
            except socket.timeout:
                print(f"No UDP data received for {uds_server_path}")
                assert False, f"No UDP data received for {uds_server_path}"
            finally:
                s.close()
    finally:
        uds_sock.close()


def test_dl_uds_to_udp_high_rate():
    """Test: DL_EL_H UDS can handle high-rate download (>=1Gbps) via FSL to UDP."""

    gcom_udp_ip = "127.0.0.1"
    gcom_udp_port = 9010
    instance = int(os.environ.get("STATEFULSET_INDEX", "-1"))
    prefix = f"/tmp/sensor-{instance}/" if instance >= 0 else "/tmp/"
    uds_server_path = prefix + "DL_EL_H"
    data_size = 100 * 1024 * 1024  # 100MB
    chunk_size = UDP_MTU  # 65KB per chunk (fits in uint16)
    total_chunks = math.ceil(data_size / chunk_size)
    payload = b"A" * data_size

    # Start UDP receiver (GSL)
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    # Increase socket receive buffer size (e.g., 256MB)
    s.setsockopt(socket.SOL_SOCKET, socket.SO_RCVBUF, 256 * 1024 * 1024)
    s.bind((gcom_udp_ip, gcom_udp_port))
    s.settimeout(10)

    received_bytes = 0
    received_chunks = 0
    start_time = time.time()

    # Synchronization event to ensure receiver is ready
    receiver_ready = threading.Event()

    def udp_receiver():
        nonlocal received_bytes, received_chunks
        receiver_ready.set()  # Signal that receiver is ready
        try:
            while received_bytes < data_size:
                data, addr = s.recvfrom(UDP_MTU)
                received_bytes += len(data)
                received_chunks += 1
        except socket.timeout:
            pass

    recv_thread = threading.Thread(target=udp_receiver)
    recv_thread.start()
    receiver_ready.wait()  # Wait until receiver signals readiness

    # Send data via UDS in chunks using a single socket
    init_uds_path(uds_server_path)
    uds_sock = socket.socket(socket.AF_UNIX, socket.SOCK_DGRAM)
    try:
        for i in range(total_chunks):
            chunk = payload[i * chunk_size : (i + 1) * chunk_size]
            opcode = 42
            error = 0
            seq_id = i + 1
            length = len(chunk)
            header = struct.pack("<BBHH", opcode, error, seq_id, length)
            msg = header + chunk
            send_uds_to_fsl(uds_server_path, msg, uds_sock)
    finally:
        uds_sock.close()

    recv_thread.join(timeout=15)
    s.close()
    elapsed = time.time() - start_time
    mbps = (received_bytes * 8) / (elapsed * 1e6)
    loss = data_size - received_bytes
    print(
        f"Received {received_bytes} bytes in {elapsed:.2f}s, {mbps:.2f} Mbps, {received_chunks} chunks"
    )
    if loss > chunk_size:
        print(f"ERROR: Lost {loss} bytes (>{chunk_size} bytes, more than one chunk)")
        assert False, f"Significant data loss: {received_bytes} < {data_size}"
    elif loss > 0:
        print(f"WARNING: Lost {loss} bytes (<={chunk_size} bytes, likely 1 chunk lost)")
    assert mbps >= 1000, f"Rate too low: {mbps:.2f} Mbps < 1000 Mbps"


def main():
    print("--- Integration Test: UDP to UDS ---")
    test_ul_udp_to_uds()
    print("--- Integration Test: UDS to UDP ---")
    test_dl_uds_to_udp()
    print("--- Integration Test: UDS to UDP High Rate ---")
    test_dl_uds_to_udp_high_rate()
    print("All integration tests passed.")


if __name__ == "__main__":
    main()
