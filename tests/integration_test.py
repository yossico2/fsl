"""
Integration tests for FSL using Python sockets.
Simulates GSL (UDP client) and two apps (UDS clients/servers).
"""

import socket
import os
import time
import struct
import threading


def send_udp_to_fcom(opcode, payload, udp_ip, udp_port):
    """Simulate GSL: Send UDP packet to FSL with header and payload."""
    # Example header: opcode (uint16), length (uint16), id (uint32)
    msg_seq_id = 1
    header = struct.pack("<HHI", opcode, len(payload), msg_seq_id)
    packet = header + payload.encode()
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    s.sendto(packet, (udp_ip, udp_port))
    s.close()


def send_uds_to_fcom(uds_path, payload):
    """Simulate app: Send payload to FSL UDS server socket."""
    s = socket.socket(socket.AF_UNIX, socket.SOCK_DGRAM)
    s.sendto(payload.encode(), uds_path)
    s.close()


def receive_uds(uds_path, timeout=2):
    """Simulate app: Receive from FSL UDS client socket."""
    if os.path.exists(uds_path):
        os.unlink(uds_path)
    s = socket.socket(socket.AF_UNIX, socket.SOCK_DGRAM)
    s.bind(uds_path)
    s.settimeout(timeout)
    try:
        data, _ = s.recvfrom(4096)
        return data
    except socket.timeout:
        return None
    finally:
        s.close()
        os.unlink(uds_path)


def uds_receiver_thread(uds_path, result_container):
    data = receive_uds(uds_path)
    result_container.append(data)


def test_udp_to_uds():
    """Test: GSL sends UDP, FSL routes to correct UDS client."""
    fsl_udp_ip = "127.0.0.1"
    fsl_udp_port = 9910
    uds_client_path = "/tmp/UL_APP1"
    opcode = 1
    payload = "hello_from_gsl"
    # Start UDS receiver (app) in a thread BEFORE sending UDP
    result = []
    t = threading.Thread(target=uds_receiver_thread, args=(uds_client_path, result))
    t.start()
    time.sleep(0.5)  # Give receiver time to bind
    print("Sending UDP to FSL...")
    send_udp_to_fcom(opcode, payload, fsl_udp_ip, fsl_udp_port)
    t.join()
    received = result[0] if result else None
    assert received is not None, "No data received on UDS client socket"
    print("Received:", received)


def test_uds_to_udp():
    """Test: App sends to UDS server, FSL routes to UDP (GSL)."""
    gcom_udp_ip = "127.0.0.1"
    gcom_udp_port = 9010
    uds_server_path = "/tmp/DL_APP1_H"
    payload = "hello_from_app"
    # Start UDP receiver (GSL)
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    s.bind((gcom_udp_ip, gcom_udp_port))
    s.settimeout(2)
    print("Sending to UDS server...")
    send_uds_to_fcom(uds_server_path, payload)
    print("Receiving from UDP...")
    try:
        data, addr = s.recvfrom(4096)
        print("Received from UDP:", data)
        assert data is not None
    except socket.timeout:
        print("No UDP data received")
        assert False, "No UDP data received"
    finally:
        s.close()


def main():
    print("--- Integration Test: UDP to UDS ---")
    test_udp_to_uds()
    print("--- Integration Test: UDS to UDP ---")
    test_uds_to_udp()
    print("All integration tests passed.")


if __name__ == "__main__":
    main()
