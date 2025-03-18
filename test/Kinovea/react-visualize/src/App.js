import React, { useState, useEffect } from "react";
import {
  ScatterChart,
  Scatter,
  XAxis,
  YAxis,
  CartesianGrid,
  Tooltip,
  Legend,
  ResponsiveContainer,
} from "recharts";
import Papa from "papaparse";

function App() {
  const [activeFile, setActiveFile] = useState("lateral_wheels");
  const [data, setData] = useState({});
  const [isLoading, setIsLoading] = useState(true);
  const [loadError, setLoadError] = useState(null);

  // File metadata for our datasets
  const fileInfo = {
    lateral_base: {
      displayName: "Lateral Base",
      fileName: "lateral base.csv",
      timeColumn: "Time (ms)",
      trials: ["0", "1", "2"],
      columnMappings: {
        "Head_base0_horizontal-displacement": {
          type: "h",
          part: "head",
          trial: "0",
        },
        "tail_base0_horizontal-displacement": {
          type: "h",
          part: "tail",
          trial: "0",
        },
        "Head_base0_vertical-displacement": {
          type: "v",
          part: "head",
          trial: "0",
        },
        "tail_base0_vertical-displacement": {
          type: "v",
          part: "tail",
          trial: "0",
        },
        "Tail_base1_horizontal-displacement": {
          type: "h",
          part: "tail",
          trial: "1",
        },
        "Head_base1_horizontal-displacement": {
          type: "h",
          part: "head",
          trial: "1",
        },
        "Tail_base1_vertical-displacement": {
          type: "v",
          part: "tail",
          trial: "1",
        },
        "Head_base1_vertical-displacement": {
          type: "v",
          part: "head",
          trial: "1",
        },
        "Tail_base2_horizontal-displacement": {
          type: "h",
          part: "tail",
          trial: "2",
        },
        "Head_base2_horizontal-displacement": {
          type: "h",
          part: "head",
          trial: "2",
        },
        "Tail_base2_vertical-displacement": {
          type: "v",
          part: "tail",
          trial: "2",
        },
        "Head_base2_vertical-displacement": {
          type: "v",
          part: "head",
          trial: "2",
        },
      },
    },
    lateral_scales: {
      displayName: "Lateral Scales",
      fileName: "lateral scales.csv",
      timeColumn: "time (ms)",
      trials: ["0", "1", "3"],
      columnMappings: {
        "tail_scales-0-horizontal-displacement": {
          type: "h",
          part: "tail",
          trial: "0",
        },
        "head_scales-0-horizontal-displacement": {
          type: "h",
          part: "head",
          trial: "0",
        },
        "tail_scales-0-vertical-displacement": {
          type: "v",
          part: "tail",
          trial: "0",
        },
        "head_scales-0-vertical-displacement": {
          type: "v",
          part: "head",
          trial: "0",
        },
        "tail_scales-1-horizontal-displacement": {
          type: "h",
          part: "tail",
          trial: "1",
        },
        "head_scales-1-horizontal-displacement": {
          type: "h",
          part: "head",
          trial: "1",
        },
        "tail_scales-1-vertical-displacement": {
          type: "v",
          part: "tail",
          trial: "1",
        },
        "head_scales-1-vertical-displacement": {
          type: "v",
          part: "head",
          trial: "1",
        },
        "tail_scales-3-horizontal-displacement": {
          type: "h",
          part: "tail",
          trial: "3",
        },
        "head_scales-3-horizontal-displacement": {
          type: "h",
          part: "head",
          trial: "3",
        },
        "tail_scales-3-vertical-displacement": {
          type: "v",
          part: "tail",
          trial: "3",
        },
        "head_scales-3-vertical-displacement": {
          type: "v",
          part: "head",
          trial: "3",
        },
      },
    },
    lateral_wheels: {
      displayName: "Lateral Wheels",
      fileName: "lateral wheels.csv",
      timeColumn: "Time (ms)",
      trials: ["0", "1", "2"],
      columnMappings: {
        "lateralundulation-wheels-0-horizontal-displacement_Tail": {
          type: "h",
          part: "tail",
          trial: "0",
        },
        "lateralundulation-wheels-0-horizontal-displacement_Head": {
          type: "h",
          part: "head",
          trial: "0",
        },
        "lateralundulation-wheels-0-vertical-displacement_Tail": {
          type: "v",
          part: "tail",
          trial: "0",
        },
        "lateralundulation-wheels-0-vertical-displacement_Head": {
          type: "v",
          part: "head",
          trial: "0",
        },
        "lateralundulation-wheels-1-horizontal-displacement_Tail": {
          type: "h",
          part: "tail",
          trial: "1",
        },
        "lateralundulation-wheels-1-horizontal-displacement_Head": {
          type: "h",
          part: "head",
          trial: "1",
        },
        "lateralundulation-wheels-1-vertical-displacement_Tail": {
          type: "v",
          part: "tail",
          trial: "1",
        },
        "lateralundulation-wheels-1-vertical-displacement_Head": {
          type: "v",
          part: "head",
          trial: "1",
        },
        "lateralundulation-wheels-2-horizontal-displacement_Tail": {
          type: "h",
          part: "tail",
          trial: "2",
        },
        "lateralundulation-wheels-2-horizontal-displacement_Head": {
          type: "h",
          part: "head",
          trial: "2",
        },
        "lateralundulation-wheels-2-vertical-displacement_Tail": {
          type: "v",
          part: "tail",
          trial: "2",
        },
        "lateralundulation-wheels-2-vertical-displacement_Head": {
          type: "v",
          part: "head",
          trial: "2",
        },
      },
    },
    sidewinding_base: {
      displayName: "Sidewinding Base",
      fileName: "sidewinding base.csv",
      timeColumn: "Time (ms)",
      trials: ["0", "1", "2"],
      columnMappings: {
        "sidewinding-base-0-horizontal-displacement_Head": {
          type: "h",
          part: "head",
          trial: "0",
        },
        "sidewinding-base-0-vertical-displacement_Head": {
          type: "v",
          part: "head",
          trial: "0",
        },
        "sidewinding-base-1-horizontal-displacement_Head": {
          type: "h",
          part: "head",
          trial: "1",
        },
        "sidewinding-base-1-vertical-displacement_Head": {
          type: "v",
          part: "head",
          trial: "1",
        },
        "sidewinding-base-2-horizontal-displacement_Head": {
          type: "h",
          part: "head",
          trial: "2",
        },
        "sidewinding-base-2-vertical-displacement_Head": {
          type: "v",
          part: "head",
          trial: "2",
        },
        "sidewinding-base-0-horizontal-displacement_Tail": {
          type: "h",
          part: "tail",
          trial: "0",
        },
        "sidewinding-base-0-vertical-displacement_Tail": {
          type: "v",
          part: "tail",
          trial: "0",
        },
        "sidewinding-base-1-horizontal-displacement_Tail": {
          type: "h",
          part: "tail",
          trial: "1",
        },
        "sidewinding-base-1-vertical-displacement_Tail": {
          type: "v",
          part: "tail",
          trial: "1",
        },
        "sidewinding-base-2-horizontal-displacement_Tail": {
          type: "h",
          part: "tail",
          trial: "2",
        },
        "sidewinding-base-2-vertical-displacement_Tail": {
          type: "v",
          part: "tail",
          trial: "2",
        },
      },
    },
    sidewinding_scales_opposite: {
      displayName: "Sidewinding Scales Opposite",
      fileName: "sidewinding scales opposite.csv",
      timeColumn: "Time (ms)",
      trials: ["0", "1", "2", "3"],
      columnMappings: {
        "Tail_sidewinding-scales-0-horizontal-displacement": {
          type: "h",
          part: "tail",
          trial: "0",
        },
        "Head_sidewinding-scales-0-horizontal-displacement": {
          type: "h",
          part: "head",
          trial: "0",
        },
        "Tail_sidewinding-scales-0-vertical-displacement": {
          type: "v",
          part: "tail",
          trial: "0",
        },
        "Head_sidewinding-scales-0-vertical-displacement": {
          type: "v",
          part: "head",
          trial: "0",
        },
        "Tail_sidewinding-scales-1-horizontal-displacement": {
          type: "h",
          part: "tail",
          trial: "1",
        },
        "Head_sidewinding-scales-1-horizontal-displacement": {
          type: "h",
          part: "head",
          trial: "1",
        },
        "Tail_sidewinding-scales-1-vertical-displacement": {
          type: "v",
          part: "tail",
          trial: "1",
        },
        "Head_sidewinding-scales-1-vertical-displacement": {
          type: "v",
          part: "head",
          trial: "1",
        },
        "Tail_sidewinding-scales-2-horizontal-displacement": {
          type: "h",
          part: "tail",
          trial: "2",
        },
        "Head_sidewinding-scales-2-horizontal-displacement": {
          type: "h",
          part: "head",
          trial: "2",
        },
        "Tail_sidewinding-scales-2-vertical-displacement": {
          type: "v",
          part: "tail",
          trial: "2",
        },
        "Head_sidewinding-scales-2-vertical-displacement": {
          type: "v",
          part: "head",
          trial: "2",
        },
        "Tail_sidewinding-scales-3-horizontal-displacement": {
          type: "h",
          part: "tail",
          trial: "3",
        },
        "Head_sidewinding-scales-3-horizontal-displacement": {
          type: "h",
          part: "head",
          trial: "3",
        },
        "Tail_sidewinding-scales-3-vertical-displacement": {
          type: "v",
          part: "tail",
          trial: "3",
        },
        "Head_sidewinding-scales-3-vertical-displacement": {
          type: "v",
          part: "head",
          trial: "3",
        },
      },
    },
    sidewinding_scales: {
      displayName: "Sidewinding Scales",
      fileName: "sidewinding scales.csv",
      timeColumn: "Time (ms)",
      trials: ["0", "1", "2"],
      columnMappings: {
        "Tail_sidewinding-scales-0-horizontal-displacement": {
          type: "h",
          part: "tail",
          trial: "0",
        },
        "Head_sidewinding-scales-0-horizontal-displacement": {
          type: "h",
          part: "head",
          trial: "0",
        },
        "Tail_sidewinding-scales-0-vertical-displacement": {
          type: "v",
          part: "tail",
          trial: "0",
        },
        "Head_sidewinding-scales-0-vertical-displacement": {
          type: "v",
          part: "head",
          trial: "0",
        },
        "Tail_sidewinding-scales-1-horizontal-displacement": {
          type: "h",
          part: "tail",
          trial: "1",
        },
        "Head_sidewinding-scales-1-horizontal-displacement": {
          type: "h",
          part: "head",
          trial: "1",
        },
        "Tail_sidewinding-scales-1-vertical-displacement": {
          type: "v",
          part: "tail",
          trial: "1",
        },
        "Head_sidewinding-scales-1-vertical-displacement": {
          type: "v",
          part: "head",
          trial: "1",
        },
        "Tail_sidewinding-scales-2-horizontal-displacement": {
          type: "h",
          part: "tail",
          trial: "2",
        },
        "Head_sidewinding-scales-2-horizontal-displacement": {
          type: "h",
          part: "head",
          trial: "2",
        },
        "Tail_sidewinding-scales-2-vertical-displacement": {
          type: "v",
          part: "tail",
          trial: "2",
        },
        "Head_sidewinding-scales-2-vertical-displacement": {
          type: "v",
          part: "head",
          trial: "2",
        },
      },
    },
  };

  // Load data directly from CSV files when active file changes
  useEffect(() => {
    const loadFile = async () => {
      setIsLoading(true);
      setLoadError(null);

      try {
        const fileMetadata = fileInfo[activeFile];
        const fileName = fileMetadata.fileName;

        // Fetch the file from the public folder
        const response = await fetch(`/${fileName}`);

        if (!response.ok) {
          throw new Error(
            `Failed to fetch ${fileName}: ${response.status} ${response.statusText}`
          );
        }

        const text = await response.text();

        // Parse CSV data
        const parsedResult = Papa.parse(text, {
          header: true,
          dynamicTyping: true,
          skipEmptyLines: true,
        });

        if (parsedResult.errors && parsedResult.errors.length > 0) {
          console.warn("CSV parsing had errors:", parsedResult.errors);
        }

        // Process the data
        const processedData = processCSVData(parsedResult.data, fileMetadata);

        setData((prevData) => ({
          ...prevData,
          [activeFile]: processedData,
        }));
      } catch (error) {
        console.error(`Error loading data:`, error);
        setLoadError(error.message);

        // As a fallback, generate some sample data
        const sampleData = generateSampleData(activeFile);
        setData((prevData) => ({
          ...prevData,
          [activeFile]: sampleData,
        }));
      } finally {
        setIsLoading(false);
      }
    };

    loadFile();
  }, [activeFile]);

  // Process CSV data using the column mappings
  const processCSVData = (rawData, fileMetadata) => {
    const timeColumn = fileMetadata.timeColumn;
    const columnMappings = fileMetadata.columnMappings;

    return rawData.map((row, index) => {
      // Create a normalized row with our standard format
      const normalizedRow = {
        id: row.ID || row["Unnamed: 0"] || index,
        time: row[timeColumn] || 0,
      };

      // Map each column based on our mappings
      Object.entries(columnMappings).forEach(([originalColumn, mapping]) => {
        const normalizedKey = `${mapping.part}_${mapping.type}${mapping.trial}`;
        normalizedRow[normalizedKey] = row[originalColumn];
      });

      return normalizedRow;
    });
  };

  // Generate fallback sample data if file loading fails
  const generateSampleData = (fileKey) => {
    const fileMetadata = fileInfo[fileKey];
    const trials = fileMetadata.trials;
    const timePoints = 100;
    const result = [];

    // Generate rows of data
    for (let i = 0; i < timePoints; i++) {
      const time = i * 10;
      const row = {
        id: i,
        time,
      };

      // Generate data for each trial
      trials.forEach((trial) => {
        const phase = parseInt(trial) * 0.5; // Offset for different trials

        // Different patterns for different movement types
        let headHorizontal, headVertical, tailHorizontal, tailVertical;

        if (fileKey.includes("lateral")) {
          // Lateral undulation pattern
          headHorizontal = Math.sin(i / 10 + phase) * 0.8;
          headVertical = Math.cos(i / 12 + phase) * 0.2;
          tailHorizontal = Math.sin(i / 10 + phase - 1) * 0.7; // Phase delay for tail
          tailVertical = Math.cos(i / 12 + phase - 1) * 0.15;
        } else {
          // Sidewinding pattern
          headHorizontal =
            Math.sin(i / 8 + phase) * 0.6 + Math.cos(i / 10 + phase) * 0.3;
          headVertical =
            Math.cos(i / 8 + phase) * 0.6 + Math.sin(i / 10 + phase) * 0.3;
          tailHorizontal =
            Math.sin(i / 8 + phase - 1.5) * 0.5 +
            Math.cos(i / 10 + phase - 1.5) * 0.25;
          tailVertical =
            Math.cos(i / 8 + phase - 1.5) * 0.5 +
            Math.sin(i / 10 + phase - 1.5) * 0.25;
        }

        // Add scales effect for scale variations
        if (fileKey.includes("scales")) {
          headHorizontal *= 1.2;
          tailHorizontal *= 1.2;
        }

        // Add wheels effect for wheel variations
        if (fileKey.includes("wheels")) {
          headHorizontal *= 1.4;
          tailHorizontal *= 1.4;
        }

        // Add a small random variation
        headHorizontal += Math.random() * 0.1 - 0.05;
        headVertical += Math.random() * 0.1 - 0.05;
        tailHorizontal += Math.random() * 0.1 - 0.05;
        tailVertical += Math.random() * 0.1 - 0.05;

        // Add to row
        row[`head_h${trial}`] = parseFloat(headHorizontal.toFixed(4));
        row[`head_v${trial}`] = parseFloat(headVertical.toFixed(4));
        row[`tail_h${trial}`] = parseFloat(tailHorizontal.toFixed(4));
        row[`tail_v${trial}`] = parseFloat(tailVertical.toFixed(4));
      });

      result.push(row);
    }

    return result;
  };

  // Get all trials for a scatter plot of one type (horizontal or vertical)
  const getAllTrialsScatterData = (fileKey, type) => {
    if (!data[fileKey] || !fileInfo[fileKey]) return [];

    const trials = fileInfo[fileKey].trials;
    const result = [];

    trials.forEach((trial) => {
      // Add head data for this trial
      const headKey = `head_${type}${trial}`;
      const headData = data[fileKey]
        .filter((row) => row[headKey] !== undefined && row[headKey] !== null)
        .map((row) => ({
          time: row.time,
          value: row[headKey],
        }));

      if (headData.length > 0) {
        result.push({
          name: `Head - Trial ${trial}`,
          data: headData,
        });
      }

      // Add tail data for this trial
      const tailKey = `tail_${type}${trial}`;
      const tailData = data[fileKey]
        .filter((row) => row[tailKey] !== undefined && row[tailKey] !== null)
        .map((row) => ({
          time: row.time,
          value: row[tailKey],
        }));

      if (tailData.length > 0) {
        result.push({
          name: `Tail - Trial ${trial}`,
          data: tailData,
        });
      }
    });

    return result;
  };

  // Helper function to get colors for data series
  const getSeriesColor = (seriesName) => {
    const colorMap = {
      "Head - Trial 0": "#8884d8",
      "Tail - Trial 0": "#82ca9d",
      "Head - Trial 1": "#ff7300",
      "Tail - Trial 1": "#0088fe",
      "Head - Trial 2": "#ffc658",
      "Tail - Trial 2": "#00C49F",
      "Head - Trial 3": "#FFBB28",
      "Tail - Trial 3": "#FF8042",
    };

    return colorMap[seriesName] || "#999999";
  };

  // Render data table for the current file
  const renderDataTable = () => {
    if (!data[activeFile] || data[activeFile].length === 0) {
      return (
        <div className="no-data-message">
          No data available for the selected file.
        </div>
      );
    }

    const fileMetadata = fileInfo[activeFile];
    const rows = data[activeFile].slice(0, 10); // Show only first 10 rows for brevity

    // Determine which columns to display in the table
    const displayColumns = [
      { id: "time", name: fileMetadata.timeColumn },
      { id: "head_h0", name: "Head Horizontal (Trial 0)" },
      { id: "tail_h0", name: "Tail Horizontal (Trial 0)" },
      { id: "head_v0", name: "Head Vertical (Trial 0)" },
      { id: "tail_v0", name: "Tail Vertical (Trial 0)" },
    ];

    // Add additional trials if available
    if (fileMetadata.trials.includes("1")) {
      displayColumns.push(
        { id: "head_h1", name: "Head Horizontal (Trial 1)" },
        { id: "tail_h1", name: "Tail Horizontal (Trial 1)" },
        { id: "head_v1", name: "Head Vertical (Trial 1)" },
        { id: "tail_v1", name: "Tail Vertical (Trial 1)" }
      );
    }

    return (
      <div className="data-table-container">
        <h3>{fileMetadata.displayName} - Data Table (First 10 rows)</h3>
        <div className="table-wrapper">
          <table className="data-table">
            <thead>
              <tr>
                {displayColumns.map((column) => (
                  <th key={column.id}>{column.name}</th>
                ))}
              </tr>
            </thead>
            <tbody>
              {rows.map((row, rowIndex) => (
                <tr key={rowIndex}>
                  {displayColumns.map((column) => (
                    <td key={column.id}>
                      {column.id === "time"
                        ? row[column.id]
                        : row[column.id] !== undefined &&
                          row[column.id] !== null
                        ? row[column.id].toFixed(4)
                        : "N/A"}
                    </td>
                  ))}
                </tr>
              ))}
            </tbody>
          </table>
        </div>
        <div className="table-note">
          Showing first 10 rows out of {data[activeFile].length} total rows.
          {loadError && (
            <div className="data-error">
              Using sample data due to loading error: {loadError}
            </div>
          )}
        </div>
      </div>
    );
  };

  // Render scatter plot for horizontal displacement
  const renderHorizontalScatterPlot = () => {
    if (!data[activeFile] || data[activeFile].length === 0) return null;

    const scatterData = getAllTrialsScatterData(activeFile, "h");

    return (
      <div className="scatter-plot-container">
        <h3>{fileInfo[activeFile].displayName} - Horizontal Displacement</h3>
        <div className="chart-container">
          <ResponsiveContainer width="100%" height={400}>
            <ScatterChart margin={{ top: 10, right: 30, left: 20, bottom: 30 }}>
              <CartesianGrid strokeDasharray="3 3" />
              <XAxis
                type="number"
                dataKey="time"
                name="Time"
                label={{
                  value: "Time (ms)",
                  position: "insideBottom",
                  offset: -5,
                }}
              />
              <YAxis
                type="number"
                dataKey="value"
                name="Displacement"
                domain={[-1.5, 1.5]}
                tickFormatter={(value) => value.toFixed(2)}
                label={{
                  value: "Horizontal Displacement (cm)",
                  angle: -90,
                  position: "insideLeft",
                  offset: -10,
                  style: { textAnchor: "middle" },
                }}
              />
              <Tooltip
                formatter={(value) => value.toFixed(4)}
                labelFormatter={(label) => `Time: ${label} ms`}
              />
              <Legend verticalAlign="top" height={40} />

              {scatterData.map((series) => (
                <Scatter
                  key={series.name}
                  name={series.name}
                  data={series.data}
                  fill={getSeriesColor(series.name)}
                  line={{
                    stroke: getSeriesColor(series.name),
                    strokeWidth: 1,
                  }}
                />
              ))}
            </ScatterChart>
          </ResponsiveContainer>
        </div>
      </div>
    );
  };

  // Render scatter plot for vertical displacement
  const renderVerticalScatterPlot = () => {
    if (!data[activeFile] || data[activeFile].length === 0) return null;

    const scatterData = getAllTrialsScatterData(activeFile, "v");

    return (
      <div className="scatter-plot-container">
        <h3>{fileInfo[activeFile].displayName} - Vertical Displacement</h3>
        <div className="chart-container">
          <ResponsiveContainer width="100%" height={400}>
            <ScatterChart margin={{ top: 10, right: 30, left: 20, bottom: 30 }}>
              <CartesianGrid strokeDasharray="3 3" />
              <XAxis
                type="number"
                dataKey="time"
                name="Time"
                label={{
                  value: "Time (ms)",
                  position: "insideBottom",
                  offset: -5,
                }}
              />
              <YAxis
                type="number"
                dataKey="value"
                name="Displacement"
                domain={[-1.5, 1.5]}
                tickFormatter={(value) => value.toFixed(2)}
                label={{
                  value: "Vertical Displacement (cm)",
                  angle: -90,
                  position: "insideLeft",
                  offset: -10,
                  style: { textAnchor: "middle" },
                }}
              />
              <Tooltip
                formatter={(value) => value.toFixed(4)}
                labelFormatter={(label) => `Time: ${label} ms`}
              />
              <Legend verticalAlign="top" height={40} />

              {scatterData.map((series) => (
                <Scatter
                  key={series.name}
                  name={series.name}
                  data={series.data}
                  fill={getSeriesColor(series.name)}
                  line={{
                    stroke: getSeriesColor(series.name),
                    strokeWidth: 1,
                  }}
                />
              ))}
            </ScatterChart>
          </ResponsiveContainer>
        </div>
      </div>
    );
  };

  // Styles for the component
  const styles = {
    container: {
      fontFamily: "Arial, sans-serif",
      maxWidth: "1200px",
      margin: "0 auto",
      padding: "20px",
    },
    header: {
      textAlign: "center",
      marginBottom: "20px",
    },
    fileSelector: {
      display: "flex",
      justifyContent: "center",
      flexWrap: "wrap",
      gap: "10px",
      marginBottom: "20px",
    },
    fileButton: {
      padding: "8px 12px",
      borderRadius: "4px",
      cursor: "pointer",
      border: "none",
      fontSize: "14px",
    },
    activeFileButton: {
      backgroundColor: "#4299e1",
      color: "white",
    },
    inactiveFileButton: {
      backgroundColor: "#e2e8f0",
      color: "#333",
    },
    content: {
      display: "flex",
      flexDirection: "column",
      gap: "20px",
    },
    loadingContainer: {
      textAlign: "center",
      padding: "40px",
      backgroundColor: "#f0f9ff",
      borderRadius: "8px",
      marginBottom: "20px",
    },
    dataTableContainer: {
      backgroundColor: "white",
      borderRadius: "8px",
      padding: "20px",
      boxShadow: "0 2px 8px rgba(0,0,0,0.1)",
      overflow: "auto",
    },
    tableWrapper: {
      overflowX: "auto",
    },
    dataTable: {
      width: "100%",
      borderCollapse: "collapse",
      fontSize: "14px",
    },
    tableNote: {
      fontSize: "12px",
      color: "#666",
      marginTop: "10px",
      textAlign: "right",
    },
    scatterContainer: {
      backgroundColor: "white",
      borderRadius: "8px",
      padding: "20px",
      boxShadow: "0 2px 8px rgba(0,0,0,0.1)",
      marginBottom: "20px",
    },
    dataError: {
      color: "#e53e3e",
      marginTop: "8px",
      fontStyle: "italic",
    },
  };

  return (
    <div style={styles.container}>
      <h2 style={styles.header}>Robot Movement Data Visualization</h2>

      <div style={styles.fileSelector}>
        {Object.keys(fileInfo).map((fileKey) => (
          <button
            key={fileKey}
            onClick={() => setActiveFile(fileKey)}
            style={{
              ...styles.fileButton,
              ...(activeFile === fileKey
                ? styles.activeFileButton
                : styles.inactiveFileButton),
            }}
          >
            {fileInfo[fileKey].displayName}
          </button>
        ))}
      </div>

      {isLoading ? (
        <div style={styles.loadingContainer}>
          <h3>Loading Data...</h3>
          <p>
            Please wait while the robot movement data is being loaded from CSV
            files.
          </p>
        </div>
      ) : (
        <div style={styles.content}>
          <div style={styles.dataTableContainer}>{renderDataTable()}</div>

          <div style={styles.scatterContainer}>
            {renderHorizontalScatterPlot()}
          </div>

          <div style={styles.scatterContainer}>
            {renderVerticalScatterPlot()}
          </div>
        </div>
      )}
    </div>
  );
}

export default App;
