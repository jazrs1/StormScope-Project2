
import json
import os
import subprocess
from pathlib import Path

from flask import Flask, jsonify, request, send_from_directory

ROOT = Path(__file__).resolve().parents[1]
WEB_DIR = ROOT / "web"
DRIVER = ROOT / "build" / ("storm_cli.exe" if os.name == "nt" else "storm_cli")

ALGORITHMS = {"quick", "merge", "both"}
FIELDS = {"damage", "injuries", "date"}
MAX_LIMIT = 100
TIMEOUT_SECONDS = 120

app = Flask(__name__, static_folder=str(WEB_DIR), static_url_path="")

_options_cache = None  


class DriverError(Exception):
    def __init__(self, message, status):
        super().__init__(message)
        self.message = message
        self.status = status


def run_driver(args):

    if not DRIVER.exists():
        raise DriverError(
            f"C++ driver not found at build/{DRIVER.name}. Build it first (see README).", 500
        )
    try:
        result = subprocess.run(
            [str(DRIVER), *args],
            cwd=ROOT,
            capture_output=True,
            text=True,
            encoding="utf-8",
            errors="replace",
            timeout=TIMEOUT_SECONDS,
            shell=False,
        )
    except subprocess.TimeoutExpired:
        raise DriverError("The C++ driver took too long and was stopped.", 504)
    except OSError as exc:
        raise DriverError(f"Could not start the C++ driver: {exc}", 500)

    if result.returncode != 0:
        detail = result.stderr.strip().splitlines()
        message = detail[-1] if detail else f"exit code {result.returncode}"
        status = 400 if result.returncode == 2 else 500
        raise DriverError(f"C++ driver error: {message}", status)

    try:
        return json.loads(result.stdout)
    except json.JSONDecodeError:
        raise DriverError("The C++ driver returned invalid JSON.", 502)


def get_options():
    global _options_cache
    if _options_cache is None:
        _options_cache = run_driver(["--options"])
    return _options_cache


def error_response(message, status):
    return jsonify({"error": message}), status


@app.errorhandler(DriverError)
def handle_driver_error(err):
    return error_response(err.message, err.status)


@app.get("/")
def index():
    return send_from_directory(WEB_DIR, "index.html")


@app.get("/api/options")
def api_options():
    return jsonify(get_options())


@app.get("/api/events")
def api_events():
    params = request.args

    algorithm = params.get("algorithm", "quick")
    field = params.get("field", "damage")
    state = params.get("state", "")
    event_type = params.get("type", "")
    year_text = params.get("year", "0") or "0"
    offset_text = params.get("offset", "0") or "0"
    limit_text = params.get("limit", "50") or "50"

    if algorithm not in ALGORITHMS:
        return error_response("algorithm must be quick, merge, or both.", 400)
    if field not in FIELDS:
        return error_response("field must be damage, injuries, or date.", 400)

    try:
        year = int(year_text)
        offset = int(offset_text)
        limit = int(limit_text)
    except ValueError:
        return error_response("year, offset, and limit must be whole numbers.", 400)

    if offset < 0:
        return error_response("offset cannot be negative.", 400)
    if not 1 <= limit <= MAX_LIMIT:
        return error_response(f"limit must be between 1 and {MAX_LIMIT}.", 400)


    options = get_options()
    if state and state not in options["states"]:
        return error_response(f"Unknown state: {state}", 400)
    if event_type and event_type not in options["types"]:
        return error_response(f"Unknown event type: {event_type}", 400)
    if year != 0 and year not in options["years"]:
        return error_response(f"Unknown year: {year}", 400)

    args = [
        "--algorithm", algorithm,
        "--field", field,
        "--year", str(year),
        "--offset", str(offset),
        "--limit", str(limit),
    ]
    if state:
        args += ["--state", state]
    if event_type:
        args += ["--type", event_type]

    return jsonify(run_driver(args))