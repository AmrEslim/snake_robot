import React, { useState, useEffect } from "react";
import {
  ComposedChart,
  LineChart,
  Line,
  ScatterChart,
  Scatter,
  XAxis,
  YAxis,
  CartesianGrid,
  Tooltip,
  Legend,
} from "recharts";

import Papa from "papaparse";
import _ from "lodash";

const colors = [
  "#8884d8",
  "#82ca9d",
  "#ffc658",
  "#ff8042",
  "#0088fe",
  "#00C49F",
];

// Custom tooltip component for charts
const CustomTooltip = ({ active, payload, label }) => {
  if (active && payload && payload.length) {
    return (
      <div
        style={{
          backgroundColor: "white",
          padding: "5px 10px",
          border: "1px solid #ccc",
          borderRadius: "5px",
        }}
      >
        <p style={{ fontWeight: "bold" }}>{`Time: ${Number(label).toFixed(
          2
        )}s`}</p>
        {payload.map((entry, index) => (
          <p key={index} style={{ color: entry.color }}>
            {`${entry.name}: ${Number(entry.value).toFixed(2)}A`}
          </p>
        ))}
      </div>
    );
  }
  return null;
};

const DataVisualization = () => {
  const [data, setData] = useState({
    idle: { raw: [], avg: [] },
    lateralBase: { raw: [], avg: [] },
    lateralScales: { raw: [], avg: [] },
    lateralWheels: { raw: [], avg: [] },
    sidewindingBase: { raw: [], avg: [] },
    sidewindingScales: { raw: [], avg: [] },
  });

  const [stats, setStats] = useState({});
  const [loading, setLoading] = useState(true);
  const [activeTab, setActiveTab] = useState("idle");
  const [compareMode, setCompareMode] = useState(false);

  // Common Y-axis domain for all charts
  const yAxisDomain = [0, 7]; // Fixed y-axis from 0 to 7A for all charts
  const xAxisDomain = [0, 10]; // Fixed x-axis from 0 to 10s for consistent visualization

  useEffect(() => {
    // Use already uploaded files
    processAllFiles();
  }, []);

  // Process all uploaded files
  const processAllFiles = () => {
    const fileNames = {
      idle: "merged_idle_data.csv",
      lateralBase: "Merged_Lateral_Base_Data.csv",
      lateralScales: "Merged_Lateral_Scales_Data.csv",
      lateralWheels: "Merged_Lateral_Wheels_Data.csv",
      sidewindingBase: "Merged_Sidewinding_Base_Data.csv",
      sidewindingScales: "Merged_Sidewinding_Scales_Data.csv",
    };

    Promise.all(
      Object.entries(fileNames).map(([key, fileName]) =>
        fetch(fileName)
          .then((response) => {
            if (!response.ok) {
              throw new Error(`Failed to fetch ${fileName}`);
            }
            return response.text();
          })
          .then((content) => {
            return { key, content };
          })
          .catch((error) => {
            console.error(`Error loading ${fileName}:`, error);
            return { key, content: null };
          })
      )
    ).then((results) => {
      const processedData = {};
      const statsData = {};

      results.forEach(({ key, content }) => {
        if (content) {
          const parsedData = Papa.parse(content, {
            header: true,
            dynamicTyping: true,
            skipEmptyLines: true,
          });

          if (parsedData && parsedData.data && parsedData.data.length > 0) {
            const columns = parsedData.meta.fields;
            processedData[key] = processData(parsedData.data, columns);
            statsData[key] = calculateStats(parsedData.data, columns);
          }
        }
      });

      setData({
        idle: processedData.idle || { raw: [], avg: [] },
        lateralBase: processedData.lateralBase || { raw: [], avg: [] },
        lateralScales: processedData.lateralScales || { raw: [], avg: [] },
        lateralWheels: processedData.lateralWheels || { raw: [], avg: [] },
        sidewindingBase: processedData.sidewindingBase || { raw: [], avg: [] },
        sidewindingScales: processedData.sidewindingScales || {
          raw: [],
          avg: [],
        },
      });

      setStats(statsData);
      setLoading(false);
    });
  };

  // Helper function to process data for visualization
  function processData(data, columns) {
    // Skip the first column (Time)
    const dataColumns = columns.filter((col) => col !== "Time(s)");

    // Prepare raw data for scatter plot
    const rawData = [];
    data.forEach((row) => {
      if (row["Time(s)"] <= 10) {
        // Limit to first 10 seconds for consistent visualization
        dataColumns.forEach((col) => {
          if (row[col] !== null && !isNaN(row[col])) {
            rawData.push({
              time: row["Time(s)"],
              current: row[col],
              run: col,
            });
          }
        });
      }
    });

    // Calculate average by time
    const timeGroups = _.groupBy(
      data.filter((row) => row["Time(s)"] <= 10),
      "Time(s)"
    );
    const avgData = Object.entries(timeGroups)
      .map(([time, rows]) => {
        const currents = [];
        dataColumns.forEach((col) => {
          rows.forEach((row) => {
            if (row[col] !== null && !isNaN(row[col])) {
              currents.push(row[col]);
            }
          });
        });

        return {
          time: parseFloat(time),
          avgCurrent: currents.length > 0 ? _.mean(currents) : 0,
        };
      })
      .sort((a, b) => a.time - b.time);

    return { raw: rawData, avg: avgData };
  }

  // Helper function to calculate statistics
  function calculateStats(data, columns) {
    const dataColumns = columns.filter((col) => col !== "Time(s)");
    const allValues = [];

    dataColumns.forEach((col) => {
      data.forEach((row) => {
        if (row[col] !== null && !isNaN(row[col])) {
          allValues.push(row[col]);
        }
      });
    });

    if (allValues.length === 0) return {};

    return {
      min: Math.min(...allValues),
      max: Math.max(...allValues),
      avg: _.mean(allValues),
      median: _.sortBy(allValues)[Math.floor(allValues.length / 2)],
      stdDev: Math.sqrt(
        _.sum(allValues.map((v) => Math.pow(v - _.mean(allValues), 2))) /
          allValues.length
      ),
    };
  }

  const tabData = {
    idle: { title: "Idle Mode", data: data.idle, stats: stats.idle },
    lateralWheels: {
      title: "Lateral Undulation (Wheels)",
      data: data.lateralWheels,
      stats: stats.lateralWheels,
    },
    lateralScales: {
      title: "Lateral Undulation (Scales)",
      data: data.lateralScales,
      stats: stats.lateralScales,
    },
    lateralBase: {
      title: "Lateral Base",
      data: data.lateralBase,
      stats: stats.lateralBase,
    },
    sidewindingBase: {
      title: "Sidewinding Base",
      data: data.sidewindingBase,
      stats: stats.sidewindingBase,
    },
    sidewindingScales: {
      title: "Sidewinding (Scales)",
      data: data.sidewindingScales,
      stats: stats.sidewindingScales,
    },
  };

  // Check if all required files are loaded and there's data to display
  const isDataReady = Object.values(data).some(
    (d) => d.raw.length > 0 && d.avg.length > 0
  );

  const renderStatsCard = (stats, title) => {
    if (!stats) return null;

    return (
      <div
        style={{
          backgroundColor: "white",
          padding: "16px",
          borderRadius: "8px",
          boxShadow: "0 2px 4px rgba(0,0,0,0.1)",
          marginBottom: "16px",
        }}
      >
        <h3
          style={{ fontSize: "18px", fontWeight: "600", marginBottom: "8px" }}
        >
          {title} Statistics
        </h3>
        <div
          style={{
            display: "grid",
            gridTemplateColumns: "repeat(5, 1fr)",
            gap: "16px",
          }}
        >
          <div
            style={{
              backgroundColor: "#f9f9f9",
              padding: "12px",
              borderRadius: "4px",
            }}
          >
            <div style={{ fontSize: "14px", color: "#666" }}>Min (A)</div>
            <div style={{ fontSize: "18px", fontWeight: "600" }}>
              {stats.min?.toFixed(2)}
            </div>
          </div>
          <div
            style={{
              backgroundColor: "#f9f9f9",
              padding: "12px",
              borderRadius: "4px",
            }}
          >
            <div style={{ fontSize: "14px", color: "#666" }}>Max (A)</div>
            <div style={{ fontSize: "18px", fontWeight: "600" }}>
              {stats.max?.toFixed(2)}
            </div>
          </div>
          <div
            style={{
              backgroundColor: "#f9f9f9",
              padding: "12px",
              borderRadius: "4px",
            }}
          >
            <div style={{ fontSize: "14px", color: "#666" }}>Average (A)</div>
            <div style={{ fontSize: "18px", fontWeight: "600" }}>
              {stats.avg?.toFixed(2)}
            </div>
          </div>
          <div
            style={{
              backgroundColor: "#f9f9f9",
              padding: "12px",
              borderRadius: "4px",
            }}
          >
            <div style={{ fontSize: "14px", color: "#666" }}>Median (A)</div>
            <div style={{ fontSize: "18px", fontWeight: "600" }}>
              {stats.median?.toFixed(2)}
            </div>
          </div>
          <div
            style={{
              backgroundColor: "#f9f9f9",
              padding: "12px",
              borderRadius: "4px",
            }}
          >
            <div style={{ fontSize: "14px", color: "#666" }}>Std Dev (A)</div>
            <div style={{ fontSize: "18px", fontWeight: "600" }}>
              {stats.stdDev?.toFixed(2)}
            </div>
          </div>
        </div>
      </div>
    );
  };

  const renderChart = (chartData, title, key) => {
    if (!chartData || !chartData.raw || chartData.raw.length === 0) {
      return (
        <div
          style={{
            backgroundColor: "white",
            borderRadius: "8px",
            padding: "16px",
            marginBottom: "24px",
            boxShadow: "0 2px 4px rgba(0,0,0,0.1)",
          }}
        >
          <h3
            style={{
              fontSize: "18px",
              fontWeight: "600",
              marginBottom: "16px",
            }}
          >
            {title}
          </h3>
          <div
            style={{
              height: "320px",
              display: "flex",
              alignItems: "center",
              justifyContent: "center",
            }}
          >
            <p style={{ color: "#666" }}>No data available</p>
          </div>
        </div>
      );
    }

    return (
      <div
        style={{
          backgroundColor: "white",
          borderRadius: "8px",
          padding: "16px",
          marginBottom: "24px",
          boxShadow: "0 2px 4px rgba(0,0,0,0.1)",
        }}
      >
        <h3
          style={{ fontSize: "18px", fontWeight: "600", marginBottom: "16px" }}
        >
          {title}
        </h3>
        <div style={{ height: "320px", width: "100%" }}>
          <ComposedChart
            width={800}
            height={320}
            data={chartData.raw}
            margin={{ top: 10, right: 30, left: 20, bottom: 30 }}
          >
            <CartesianGrid strokeDasharray="3 3" />
            <XAxis
              dataKey="time"
              type="number"
              domain={xAxisDomain}
              label={{
                value: "Time (s)",
                position: "insideBottomRight",
                offset: -5,
              }}
            />
            <YAxis
              label={{
                value: "Current (A)",
                angle: -90,
                position: "insideLeft",
                offset: 10,
              }}
              domain={yAxisDomain}
            />
            <Tooltip content={<CustomTooltip />} />
            <Legend verticalAlign="top" height={36} />
            <Scatter
              name="Individual Readings"
              data={chartData.raw}
              fill="#8884d8"
              opacity={0.4}
              dataKey="current"
              xAxisKey="time"
            />
            <Line
              name="Average Current"
              data={chartData.avg}
              type="monotone"
              dataKey="avgCurrent"
              xAxisKey="time"
              stroke="#ff7300"
              dot={false}
              activeDot={false}
              strokeWidth={3}
            />
          </ComposedChart>
        </div>
      </div>
    );
  };

  const renderComparisonChart = () => {
    if (!isDataReady) {
      return (
        <div
          style={{
            backgroundColor: "white",
            borderRadius: "8px",
            padding: "16px",
            marginBottom: "24px",
            boxShadow: "0 2px 4px rgba(0,0,0,0.1)",
            textAlign: "center",
          }}
        >
          <h3
            style={{
              fontSize: "18px",
              fontWeight: "600",
              marginBottom: "16px",
            }}
          >
            Comparison of All Locomotion Modes
          </h3>
          <div
            style={{
              height: "400px",
              display: "flex",
              alignItems: "center",
              justifyContent: "center",
            }}
          >
            <p style={{ color: "#666" }}>No data available for comparison</p>
          </div>
        </div>
      );
    }

    const scatterData = [];

    const modes = [
      { key: "idle", name: "Idle Mode" },
      { key: "lateralWheels", name: "Lateral Undulation (Wheels)" },
      { key: "lateralScales", name: "Lateral Undulation (Scales)" },
      { key: "lateralBase", name: "Lateral Base" },
      { key: "sidewindingBase", name: "Sidewinding Base" },
      { key: "sidewindingScales", name: "Sidewinding (Scales)" },
    ];

    modes.forEach(({ key, name }) => {
      data[key]?.avg?.forEach((point) => {
        scatterData.push({
          time: point.time,
          avgCurrent: point.avgCurrent,
          mode: name,
        });
      });
    });

    const overallAvgLine = Object.entries(
      scatterData.reduce((acc, { time, avgCurrent }) => {
        acc[time] = acc[time] || [];
        acc[time].push(avgCurrent);
        return acc;
      }, {})
    ).map(([time, currents]) => ({
      time: Number(time),
      avgCurrent:
        currents.reduce((sum, value) => sum + value, 0) / currents.length,
    }));

    const contrastColors = [
      "#e6194b",
      "#3cb44b",
      "#ffe119",
      "#0082c8",
      "#f032e6",
      "#000075",
    ];

    return (
      <div
        style={{
          backgroundColor: "white",
          borderRadius: "8px",
          padding: "16px",
          marginBottom: "24px",
          boxShadow: "0 2px 4px rgba(0,0,0,0.1)",
        }}
      >
        <h3
          style={{ fontSize: "18px", fontWeight: "600", marginBottom: "16px" }}
        >
          Comparison of All Locomotion Modes
        </h3>
        <div style={{ overflowX: "auto", height: "550px" }}>
          <ScatterChart
            width={1100}
            height={500}
            margin={{ top: 20, right: 40, left: 20, bottom: 40 }}
          >
            <CartesianGrid strokeDasharray="3 3" />
            <XAxis
              dataKey="time"
              type="number"
              domain={xAxisDomain}
              label={{
                value: "Time (s)",
                position: "insideBottomRight",
                offset: -10,
              }}
            />
            <YAxis
              domain={yAxisDomain}
              label={{
                value: "Current (A)",
                angle: -90,
                position: "insideLeft",
                offset: 10,
              }}
            />
            <Tooltip content={<CustomTooltip />} />
            <Legend verticalAlign="top" height={50} />

            {modes.map((mode, index) => (
              <Scatter
                key={mode.key}
                name={mode.name}
                data={scatterData.filter((d) => d.mode === mode.name)}
                fill={contrastColors[index % contrastColors.length]}
                dataKey="avgCurrent"
                shape="circle"
                size={15}
              />
            ))}

            <Line
              type="linear"
              dataKey="avgCurrent"
              data={overallAvgLine}
              stroke="#000000"
              strokeWidth={2}
              dot={false}
            />
          </ScatterChart>
        </div>
      </div>
    );
  };

  // Debugging section
  const renderDebugInfo = () => {
    return (
      <div
        style={{
          margin: "20px",
          padding: "20px",
          border: "1px solid #ccc",
          backgroundColor: "#f9f9f9",
        }}
      >
        <h3>Debug Data:</h3>
        <p>Active Tab: {activeTab}</p>
        <p>Raw data points: {tabData[activeTab]?.data?.raw?.length || 0}</p>
        <p>Avg data points: {tabData[activeTab]?.data?.avg?.length || 0}</p>
        <p>Data structure:</p>
        <pre
          style={{
            maxHeight: "200px",
            overflow: "auto",
            backgroundColor: "#eee",
            padding: "10px",
          }}
        >
          {JSON.stringify(
            {
              raw: tabData[activeTab]?.data?.raw?.slice(0, 3) || [],
              avg: tabData[activeTab]?.data?.avg?.slice(0, 3) || [],
            },
            null,
            2
          )}
        </pre>
      </div>
    );
  };

  if (loading) {
    return (
      <div style={{ textAlign: "center", padding: "32px" }}>
        Loading data...
      </div>
    );
  }

  return (
    <div style={{ padding: "16px", backgroundColor: "#f5f5f5" }}>
      <h1
        style={{
          fontSize: "24px",
          fontWeight: "700",
          marginBottom: "24px",
          textAlign: "center",
        }}
      >
        Snake Robot Current Draw Analysis
      </h1>

      {/* Add debug button */}
      <div style={{ marginBottom: "20px", textAlign: "right" }}>
        <button
          onClick={() =>
            (document.getElementById("debugSection").style.display =
              document.getElementById("debugSection").style.display === "none"
                ? "block"
                : "none")
          }
          style={{
            padding: "5px 10px",
            backgroundColor: "#eee",
            border: "1px solid #ccc",
            borderRadius: "4px",
          }}
        >
          Toggle Debug Info
        </button>
      </div>

      <div id="debugSection" style={{ display: "none" }}>
        {renderDebugInfo()}
      </div>

      <div style={{ marginBottom: "24px" }}>
        <div
          style={{
            display: "flex",
            justifyContent: "space-between",
            alignItems: "center",
            marginBottom: "16px",
          }}
        >
          <h2 style={{ fontSize: "20px", fontWeight: "600" }}>
            Locomotion Mode Analysis
          </h2>
          <div style={{ display: "flex", gap: "8px" }}>
            <button
              onClick={() => setCompareMode(false)}
              style={{
                padding: "8px 16px",
                fontSize: "14px",
                fontWeight: "500",
                borderRadius: "8px",
                backgroundColor: !compareMode ? "#2563eb" : "#e5e7eb",
                color: !compareMode ? "white" : "#374151",
                border: "none",
                cursor: "pointer",
              }}
            >
              Individual Charts
            </button>
            <button
              onClick={() => setCompareMode(true)}
              style={{
                padding: "8px 16px",
                fontSize: "14px",
                fontWeight: "500",
                borderRadius: "8px",
                backgroundColor: compareMode ? "#2563eb" : "#e5e7eb",
                color: compareMode ? "white" : "#374151",
                border: "none",
                cursor: "pointer",
              }}
            >
              Comparison Chart
            </button>
          </div>
        </div>

        {compareMode ? (
          renderComparisonChart()
        ) : (
          <div>
            <div
              style={{
                display: "flex",
                flexWrap: "wrap",
                borderBottom: "1px solid #e5e7eb",
                marginBottom: "16px",
              }}
            >
              {Object.entries(tabData).map(([key, value]) => (
                <button
                  key={key}
                  onClick={() => setActiveTab(key)}
                  style={{
                    padding: "8px 16px",
                    fontSize: "14px",
                    fontWeight: "500",
                    borderTopLeftRadius: "8px",
                    borderTopRightRadius: "8px",
                    marginRight: "8px",
                    backgroundColor: activeTab === key ? "#2563eb" : "#e5e7eb",
                    color: activeTab === key ? "white" : "#374151",
                    border: "none",
                    cursor: "pointer",
                  }}
                >
                  {value.title}
                </button>
              ))}
            </div>

            {renderStatsCard(stats[activeTab], tabData[activeTab].title)}
            {renderChart(
              tabData[activeTab].data,
              `${tabData[activeTab].title} Current Draw`,
              activeTab
            )}
          </div>
        )}
      </div>

      <div style={{ marginTop: "32px" }}>
        <h2
          style={{ fontSize: "20px", fontWeight: "600", marginBottom: "16px" }}
        >
          All Locomotion Modes
        </h2>
        <div
          style={{
            display: "grid",
            gridTemplateColumns: "repeat(2, 1fr)",
            gap: "24px",
          }}
        >
          {Object.entries(tabData).map(([key, value]) => (
            <div
              key={key}
              style={{
                backgroundColor: "white",
                borderRadius: "8px",
                padding: "16px",
                boxShadow: "0 2px 4px rgba(0,0,0,0.1)",
              }}
            >
              <h3
                style={{
                  fontSize: "18px",
                  fontWeight: "600",
                  marginBottom: "12px",
                }}
              >
                {value.title}
              </h3>
              <div style={{ height: "256px" }}>
                {value.data && value.data.raw && value.data.raw.length > 0 ? (
                  <ComposedChart
                    width={400}
                    height={256}
                    data={value.data.raw}
                    margin={{ top: 5, right: 20, left: 20, bottom: 20 }}
                  >
                    <CartesianGrid strokeDasharray="3 3" />
                    <XAxis
                      dataKey="time"
                      type="number"
                      domain={xAxisDomain}
                      label={{
                        value: "Time (s)",
                        position: "insideBottom",
                        offset: -5,
                      }}
                    />
                    <YAxis
                      domain={yAxisDomain}
                      label={{
                        value: "Current (A)",
                        angle: -90,
                        position: "insideLeft",
                        offset: 5,
                      }}
                    />
                    <Tooltip content={<CustomTooltip />} />
                    <Scatter
                      name="Data"
                      data={value.data.raw}
                      fill="#8884d8"
                      opacity={0.4}
                      dataKey="current"
                      xAxisKey="time"
                    />
                    <Line
                      name="Average"
                      data={value.data.avg}
                      type="monotone"
                      dataKey="avgCurrent"
                      xAxisKey="time"
                      stroke="#ff7300"
                      dot={false}
                      strokeWidth={2.5}
                    />
                  </ComposedChart>
                ) : (
                  <div
                    style={{
                      display: "flex",
                      alignItems: "center",
                      justifyContent: "center",
                      height: "100%",
                    }}
                  >
                    <p style={{ color: "#666" }}>No data available</p>
                  </div>
                )}
              </div>
              {value.stats && (
                <div
                  style={{
                    marginTop: "12px",
                    display: "grid",
                    gridTemplateColumns: "repeat(5, 1fr)",
                    gap: "8px",
                    fontSize: "12px",
                  }}
                >
                  <div style={{ textAlign: "center" }}>
                    <div style={{ fontWeight: "600" }}>Min</div>
                    <div>{value.stats?.min?.toFixed(2) || "-"} A</div>
                  </div>
                  <div style={{ textAlign: "center" }}>
                    <div style={{ fontWeight: "600" }}>Max</div>
                    <div>{value.stats?.max?.toFixed(2) || "-"} A</div>
                  </div>
                  <div style={{ textAlign: "center" }}>
                    <div style={{ fontWeight: "600" }}>Avg</div>
                    <div>{value.stats?.avg?.toFixed(2) || "-"} A</div>
                  </div>
                  <div style={{ textAlign: "center" }}>
                    <div style={{ fontWeight: "600" }}>Median</div>
                    <div>{value.stats?.median?.toFixed(2) || "-"} A</div>
                  </div>
                  <div style={{ textAlign: "center" }}>
                    <div style={{ fontWeight: "600" }}>StdDev</div>
                    <div>{value.stats?.stdDev?.toFixed(2) || "-"} A</div>
                  </div>
                </div>
              )}
            </div>
          ))}
        </div>
      </div>
    </div>
  );
};

export default DataVisualization;
