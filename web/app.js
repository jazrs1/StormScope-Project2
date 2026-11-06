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
  empty: document.getElementById("empty"),
  tableWrap: document.getElementById("table-wrap"),
  rows: document.getElementById("rows"),
  prev: document.getElementById("prev"),
  next: document.getElementById("next"),
  pageInfo: document.getElementById("page-info"),
  comparison: document.getElementById("comparison"),
  timeQuick: document.getElementById("time-quick"),
  timeMerge: document.getElementById("time-merge"),
  comparisonNote: document.getElementById("comparison-note"),
  details: document.getElementById("details"),
  detailsList: document.getElementById("details-list"),
  detailsDescription: document.getElementById("details-description"),
};

const FIELD_LABELS = { damage: "property damage", injuries: "injuries", date: "date" };
const ALGORITHM_LABELS = { quick: "Quicksort", merge: "Merge sort", both: "Quicksort and merge sort" };

const currency = new Intl.NumberFormat("en-US", {
  style: "currency", currency: "USD", maximumFractionDigits: 0,
});
const count = new Intl.NumberFormat("en-US");


let currentQuery = null;
let latestRequestId = 0;
let optionsLoaded = false;

function setStatus(message, isError = false) {
  els.status.textContent = message;
  els.status.classList.toggle("error", isError);
}

function formatMs(ms) {
  if (ms === null || ms === undefined) return "not run";
  return `${ms.toFixed(3)} ms`;
}

function formatDamage(damage) {
  return damage === null ? "Unknown" : currency.format(damage);
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
    els.analyze.disabled = false;
    setStatus(`${count.format(options.loadedCount)} events loaded. Choose filters and press Analyze.`);
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
    render(data);
    setStatus("Done.");
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

function render(data) {
  lastData = data;
  els.results.hidden = false;

  const timing = data.algorithm === "merge" ? data.timingsMs.merge : data.timingsMs.quick;
  const timingText = data.algorithm === "both"
    ? `quicksort ${formatMs(data.timingsMs.quick)}, merge sort ${formatMs(data.timingsMs.merge)}`
    : formatMs(timing);

  els.summary.textContent =
    `${count.format(data.matchedCount)} matching events (of ${count.format(data.loadedCount)} loaded), ` +
    `ranked by ${FIELD_LABELS[data.field]} using ${ALGORITHM_LABELS[data.algorithm]}. ` +
    `Sort time: ${timingText}.`;

  els.rows.replaceChildren();
  const isEmpty = data.matchedCount === 0;
  els.empty.hidden = !isEmpty;
  els.tableWrap.hidden = isEmpty;

  data.rows.forEach((row, index) => {
    const tr = document.createElement("tr");
    const cells = [
      [String(data.offset + index + 1), ""],
      [row.date, ""],
      [row.state, ""],
      [row.location, ""],
      [row.type, ""],
      [formatDamage(row.damage), "num" + (row.damage === null ? " unknown" : "")],
      [String(row.injuries), "num"],
    ];
    for (const [text, className] of cells) {
      const td = document.createElement("td");
      td.textContent = text;
      if (className) td.className = className;
      tr.appendChild(td);
    }
    const actionCell = document.createElement("td");
    const button = document.createElement("button");
    button.type = "button";
    button.textContent = "Details";
    button.setAttribute("aria-label", `Details for event ${row.eventId}`);
    button.addEventListener("click", () => showDetails(row));
    actionCell.appendChild(button);
    tr.appendChild(actionCell);
    els.rows.appendChild(tr);
  });

  renderComparison(data);
}

function renderComparison(data) {
  if (data.algorithm !== "both") {
    els.comparison.hidden = true;
    return;
  }
  els.comparison.hidden = false;
  els.timeQuick.textContent = formatMs(data.timingsMs.quick);
  els.timeMerge.textContent = formatMs(data.timingsMs.merge);
  els.comparisonNote.textContent =
    `Single run on ${count.format(data.matchedCount)} records. Each algorithm sorted its own identical ` +
    `copy of the filtered data, and only the sort call was timed (not CSV loading, filtering, ` +
    `process startup, or page transfer). Both produced ` +
    `${data.ordersMatch ? "the same ranked order" : "DIFFERENT orders"}. ` +
    `See the team's repeated benchmarks for performance conclusions.`;
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
  els.pageInfo.textContent =
    `Showing ${count.format(start)}–${count.format(end)} of ${count.format(lastData.matchedCount)}`;
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
    ["Injuries", String(row.injuries)],
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