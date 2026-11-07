"use strict";

const PAGE_SIZE = 50;

const els = {
  form: document.getElementById("search-form"),
  state: document.getElementById("state"),
  year: document.getElementById("year"),
  type: document.getElementById("type"),
  field: document.getElementById("field"),
  analyze: document.getElementById("analyze"),
  status: document.getElementById("status"),
  results: document.getElementById("results"),
  summary: document.getElementById("summary"),
  timing: document.getElementById("timing"),
  timingRows: document.getElementById("timing-rows"),
  timingNote: document.getElementById("timing-note"),
  empty: document.getElementById("empty"),
  tableWrap: document.getElementById("table-wrap"),
  rows: document.getElementById("rows"),
  prev: document.getElementById("prev"),
  next: document.getElementById("next"),
  pageInfo: document.getElementById("page-info"),
  details: document.getElementById("details"),
  detailsList: document.getElementById("details-list"),
  detailsDescription: document.getElementById("details-description"),
};

const FIELD_LABELS = {
  damage: "property damage (highest first)",
  injuries: "injuries (most first)",
  date: "date (newest first)",
};

const currency = new Intl.NumberFormat("en-US", {
  style: "currency", currency: "USD", maximumFractionDigits: 0,
});
const count = new Intl.NumberFormat("en-US");

let currentQuery = null;
let latestRequestId = 0;
let optionsLoaded = false;
let loadedCount = null;

function setStatus(message, isError = false) {
  els.status.textContent = message;
  els.status.classList.toggle("error", isError);
}

function loadedText() {
  return loadedCount === null ? "" : `${count.format(loadedCount)} events loaded.`;
}

function formatMs(ms, resolutionMs) {
  if (ms === null || ms === undefined) return "not run";
  if (ms === 0) {
    const tick = resolutionMs > 0 ? `${Number(resolutionMs.toPrecision(2))} ms` : "one tick";
    return `below timer resolution (<${tick})`;
  }
  if (ms < 0.001) return "<0.001 ms";
  return `${ms.toFixed(3)} ms`;
}

function formatDamage(damage) {
  return damage === null ? "Unknown" : currency.format(damage);
}

function plural(n, word) {
  return `${count.format(n)} ${word}${n === 1 ? "" : "s"}`;
}

function addOptions(select, values) {
  for (const value of values) {
    const option = document.createElement("option");
    option.value = String(value);
    option.textContent = String(value);
    select.appendChild(option);
  }
}

async function fetchJson(url) {
  let response;
  try {
    response = await fetch(url);
  } catch {
    throw new Error("Could not reach the StormScope server. Is it running?");
  }
  let data = null;
  try {
    data = await response.json();
  } catch {

  }
  if (!response.ok) {
    const message = data && data.error ? data.error : `Server error (HTTP ${response.status}).`;
    throw new Error(message);
  }
  if (data === null) throw new Error("The server returned an unreadable response.");
  return data;
}

async function loadOptions() {
  try {
    const options = await fetchJson("/api/options");
    addOptions(els.state, options.states);
    addOptions(els.year, options.years);
    addOptions(els.type, options.types);
    optionsLoaded = true;
    loadedCount = options.loadedCount;
    els.analyze.disabled = false;
    setStatus(`${loadedText()} Choose filters and press Analyze.`);
  } catch (err) {
    setStatus(`Could not load filter options: ${err.message}`, true);
  }
}

function readForm() {
  return {
    state: els.state.value,
    year: els.year.value,
    type: els.type.value,
    field: els.field.value,
    algorithm: els.form.elements.algorithm.value,
  };
}

function setBusy(busy) {
  els.analyze.disabled = busy || !optionsLoaded;
  if (busy) {
    els.prev.disabled = true;
    els.next.disabled = true;
  }
}

async function runQuery(query, offset) {
  const requestId = ++latestRequestId;
  const params = new URLSearchParams({ ...query, offset: String(offset), limit: String(PAGE_SIZE) });

  setBusy(true);
  setStatus("Loading… (the C++ driver reloads both CSV files for each request)");

  try {
    const data = await fetchJson(`/api/events?${params}`);
    if (requestId !== latestRequestId) return;
    currentQuery = query;
    loadedCount = data.loadedCount;
    render(data);
    setStatus(loadedText());
  } catch (err) {
    if (requestId !== latestRequestId) return;
    setStatus(err.message, true);
  } finally {
    if (requestId === latestRequestId) {
      setBusy(false);
      updatePager();
    }
  }
}

let lastData = null;

function addCell(tr, text, className) {
  const td = document.createElement("td");
  td.textContent = text;
  if (className) td.className = className;
  tr.appendChild(td);
  return td;
}

function render(data) {
  lastData = data;
  els.results.hidden = false;

  els.summary.textContent =
    `${plural(data.matchedCount, "matching event")}, ranked by ${FIELD_LABELS[data.field]}`;

  const isEmpty = data.matchedCount === 0;
  els.empty.hidden = !isEmpty;
  els.tableWrap.hidden = isEmpty;
  els.timing.hidden = isEmpty;

  renderTiming(data);

  els.rows.replaceChildren();
  data.rows.forEach((row, index) => {
    const tr = document.createElement("tr");
    addCell(tr, String(data.offset + index + 1), "num");
    addCell(tr, row.date);
    addCell(tr, row.state, "wrap");
    addCell(tr, row.location, "wrap");
    addCell(tr, row.type, "wrap");
    addCell(tr, formatDamage(row.damage), row.damage === null ? "num unknown" : "num");
    addCell(tr, count.format(row.injuries), "num");

    const actionCell = addCell(tr, "");
    const button = document.createElement("button");
    button.type = "button";
    button.textContent = "Details";
    button.setAttribute("aria-label", `Details for event ${row.eventId}`);
    button.addEventListener("click", () => showDetails(row));
    actionCell.appendChild(button);

    els.rows.appendChild(tr);
  });
}

function renderTiming(data) {
  const runs = [];
  if (data.timingsMs.quick !== null) runs.push(["Quicksort", data.timingsMs.quick]);
  if (data.timingsMs.merge !== null) runs.push(["Merge sort", data.timingsMs.merge]);

  els.timingRows.replaceChildren();
  for (const [name, ms] of runs) {
    const tr = document.createElement("tr");
    addCell(tr, name);
    addCell(tr, count.format(data.matchedCount), "num");
    addCell(tr, formatMs(ms, data.timerResolutionMs), "num");
    els.timingRows.appendChild(tr);
  }

  let note = "Single run; only the sort call is timed (not file loading or page transfer).";
  if (data.algorithm === "both") {
    note += data.ordersMatch
      ? " Both algorithms produced the same order."
      : " Warning: the two algorithms produced different orders.";
  }
  els.timingNote.textContent = note;
}

function updatePager() {
  if (!lastData || lastData.matchedCount === 0) {
    els.prev.disabled = true;
    els.next.disabled = true;
    els.pageInfo.textContent = "";
    return;
  }
  const start = lastData.offset + 1;
  const end = lastData.offset + lastData.rows.length;
  els.pageInfo.textContent = `Rows ${count.format(start)}–${count.format(end)}`;
  els.prev.disabled = lastData.offset === 0;
  els.next.disabled = end >= lastData.matchedCount;
}

function showDetails(row) {
  const items = [
    ["Event ID", String(row.eventId)],
    ["Date", row.date],
    ["State", row.state],
    ["Location", row.location],
    ["Event type", row.type],
    ["Property damage", formatDamage(row.damage)],
    ["Injuries", count.format(row.injuries)],
  ];
  els.detailsList.replaceChildren();
  for (const [term, value] of items) {
    const dt = document.createElement("dt");
    dt.textContent = term;
    const dd = document.createElement("dd");
    dd.textContent = value;
    els.detailsList.append(dt, dd);
  }
  els.detailsDescription.textContent = row.description || "No narrative was recorded for this event.";
  els.details.showModal();
}

els.form.addEventListener("submit", (event) => {
  event.preventDefault();
  runQuery(readForm(), 0);
});

els.prev.addEventListener("click", () => {
  if (currentQuery && lastData) runQuery(currentQuery, Math.max(0, lastData.offset - PAGE_SIZE));
});

els.next.addEventListener("click", () => {
  if (currentQuery && lastData) runQuery(currentQuery, lastData.offset + PAGE_SIZE);
});

loadOptions();