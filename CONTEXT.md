# FSL Project Development Log

**Date:** 9 December 2025  
**Branch:** `lilo`  
**Repository:** `fsl`  
**Developer:** yossico2

---

## Key Architecture

- **C++17, CMake, TinyXML2**
- Modular: `App` class manages UDP and multiple UDS sockets (server/client)
- `config.xml` defines UDP, UDS servers/clients, and opcode-to-UDS mapping

---

## Recent Fixes & Progress

- Fixed XML parsing errors (all attribute values in `config.xml` are now quoted)
- Improved error reporting in `config.cpp` (TinyXML2 error string and file path printed on failure)
- Automatic copying of `config.xml` to build directory after build (via CMake)
- Refactored UDS socket class to `UdsSocket` (used for both server and client)
- All code and file references updated to match new naming

---

## Testing & Validation

- **UDP → UDS client:**  
  Use `socat` or Python to send UDP packets with different opcodes and payloads to FSL UDP port. Confirm correct routing to UDS client.
- **UDS server → UDP:**  
  Use `socat` or Python to send data to each UDS server socket. Confirm UDP packet sent by FSL contains valid `FslGslHeader` (opcode=0) and payload.

---

## Robustness & Logging

- Add logs for each message routed (show opcode, source, destination)
- Log errors (unknown opcode, failed send/receive)
- Add checks for socket errors and handle gracefully

---

## Configuration Validation

- On startup, check all UDS mapping names in `<ul_uds_mapping>` exist in `<client>`
- Ensure all UDS server/client paths are non-empty and unique
- Print warnings or errors if any mapping is invalid

---

## Graceful Shutdown

- Catch signals (SIGINT/SIGTERM)
- Ensure all sockets are closed and UDS files are unlinked

---

## Documentation

- Update README to describe:
  - `config.xml` structure
  - How to run and test the system
  - Example test commands/scripts

---

## How to Continue

- Pull the latest code and `config.xml`
- Review this log and open tasks
- Use the same config and test scripts for validation

---

**To resume work:**  
- Continue with testing, logging, config validation, or shutdown handling as needed.
- For any area, request code, scripts, or further instructions.

---

*End of session log*
