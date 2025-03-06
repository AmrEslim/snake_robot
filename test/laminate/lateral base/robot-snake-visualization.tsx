import React, { useState, useEffect } from 'react';
import { LineChart, Line, XAxis, YAxis, CartesianGrid, Tooltip, Legend, ResponsiveContainer, ReferenceLine } from 'recharts';
import Papa from 'papaparse';

const RobotSnakeVisualization = () => {
  const [data, setData] = useState([]);
  const [loading, setLoading] = useState(true);
  const [error, setError] = useState(null);
  const [activeTab, setActiveTab] = useState('position');
  const [timeUnit, setTimeUnit] = useState('ms');
  
  useEffect(() => {
    const loadData = async () => {
      try {
        setLoading(true);
        const response = await window.fs.readFile('combined_lateral_base.csv');
        const text = new TextDecoder().decode(response);
        
        Papa.parse(text, {
          header: true,
          dynamicTyping: true,
          skipEmptyLines: true,
          complete: (results) => {
            // Process the data and convert to centimeters
            const processedData = results.data.map(row => {
              // Convert vertical deviation from meters to centimeters
              const headVerticalCm = row["Markierung 1_vertical_deviation(m)"] * 100;
              const tailVerticalCm = row["Markierung 2_vertical_deviation (m)"] * 100;
              
              // Convert horizontal speed from m/s to cm/s
              const headHorizontalSpeedCms = row["Markierung 1_horizontal_speed(meter per second)"] * 100;
              const tailHorizontalSpeedCms = row["Markierung2_horizontal_speed (meter per second)"] * 100;
              
              // Calculate the vertical distance between head and tail
              const verticalDistanceCm = headVerticalCm - tailVerticalCm;
              
              return {
                "time": row["Zeit (ms)"],
                "headVertical": headVerticalCm,
                "tailVertical": tailVerticalCm,
                "headSpeed": headHorizontalSpeedCms,
                "tailSpeed": tailHorizontalSpeedCms,
                "verticalDistance": verticalDistanceCm
              };
            });
            
            setData(processedData);
            setLoading(false);
          },
          error: (error) => {
            setError('Error parsing CSV: ' + error.message);
            setLoading(false);
          }
        });
      } catch (err) {
        setError('Error loading file: ' + err.message);
        setLoading(false);
      }
    };

    loadData();
  }, []);

  const convertTime = (time) => {
    if (timeUnit === 'sec') {
      return (time / 1000).toFixed(2);
    }
    return time;
  };

  const getTimeLabel = () => {
    return timeUnit === 'ms' ? 'Time (ms)' : 'Time (seconds)';
  };

  if (loading) {
    return <div className="flex justify-center items-center h-64">Loading data...</div>;
  }

  if (error) {
    return <div className="p-4 text-red-500">{error}</div>;
  }

  // Use every 10th data point to avoid overcrowding the chart
  const sampledData = data.filter((_, index) => index % 10 === 0);
  
  // Format time based on selected unit
  const formattedData = sampledData.map(point => ({
    ...point,
    displayTime: convertTime(point.time)
  }));

  // Calculate summary statistics
  const timeRange = {
    min: Math.min(...data.map(d => d.time)),
    max: Math.max(...data.map(d => d.time))
  };
  
  const durationSeconds = (timeRange.max - timeRange.min) / 1000;
  
  const headVerticalRange = {
    min: Math.min(...data.map(d => d.headVertical)),
    max: Math.max(...data.map(d => d.headVertical))
  };
  
  const tailVerticalRange = {
    min: Math.min(...data.map(d => d.tailVertical)),
    max: Math.max(...data.map(d => d.tailVertical))
  };
  
  const maxVerticalDistance = Math.max(...data.map(d => Math.abs(d.verticalDistance)));
  
  return (
    <div className="p-4">
      <h2 className="text-xl font-bold mb-4">Robot Snake Movement Analysis</h2>
      
      <div className="bg-blue-50 p-4 rounded mb-6">
        <h3 className="text-lg font-semibold mb-2">Key Insights</h3>
        <p className="mb-1">• This data represents the movement of a robot snake over {durationSeconds.toFixed(2)} seconds, with measurements from head (Marker 1) and tail (Marker 2).</p>
        <p className="mb-1">• The maximum vertical displacement is {Math.max(Math.abs(headVerticalRange.min), Math.abs(headVerticalRange.max), Math.abs(tailVerticalRange.min), Math.abs(tailVerticalRange.max)).toFixed(1)} cm.</p>
        <p className="mb-1">• The maximum distance between head and tail is {maxVerticalDistance.toFixed(1)} cm.</p>
      </div>
      
      <div className="mb-4">
        <div className="flex flex-wrap gap-2 mb-2">
          <button
            className={`px-4 py-2 rounded ${activeTab === 'position' ? 'bg-blue-500 text-white' : 'bg-gray-200'}`}
            onClick={() => setActiveTab('position')}
          >
            Vertical Position
          </button>
          <button
            className={`px-4 py-2 rounded ${activeTab === 'speed' ? 'bg-blue-500 text-white' : 'bg-gray-200'}`}
            onClick={() => setActiveTab('speed')}
          >
            Horizontal Speed
          </button>
          <button
            className={`px-4 py-2 rounded ${activeTab === 'distance' ? 'bg-blue-500 text-white' : 'bg-gray-200'}`}
            onClick={() => setActiveTab('distance')}
          >
            Head-Tail Distance
          </button>
          <div className="ml-auto">
            <select 
              className="px-3 py-2 border rounded"
              value={timeUnit}
              onChange={(e) => setTimeUnit(e.target.value)}
            >
              <option value="ms">Milliseconds</option>
              <option value="sec">Seconds</option>
            </select>
          </div>
        </div>
      </div>

      {activeTab === 'position' && (
        <div className="mb-8">
          <h3 className="text-lg font-semibold mb-2">Vertical Position of Head and Tail Over Time</h3>
          <ResponsiveContainer width="100%" height={400}>
            <LineChart data={formattedData} margin={{ top: 5, right: 30, left: 20, bottom: 5 }}>
              <CartesianGrid strokeDasharray="3 3" />
              <XAxis 
                dataKey="displayTime" 
                label={{ value: getTimeLabel(), position: 'insideBottomRight', offset: -10 }} 
              />
              <YAxis 
                label={{ value: 'Vertical Position (cm)', angle: -90, position: 'insideLeft' }} 
              />
              <Tooltip 
                formatter={(value) => [value.toFixed(2) + ' cm', '']}
                labelFormatter={(value) => `Time: ${value} ${timeUnit}`}
              />
              <Legend />
              <ReferenceLine y={0} stroke="#000" strokeDasharray="3 3" />
              <Line 
                type="monotone" 
                dataKey="headVertical" 
                name="Head Position" 
                stroke="#ff7300" 
                dot={false} 
                activeDot={{ r: 8 }}
              />
              <Line 
                type="monotone" 
                dataKey="tailVertical" 
                name="Tail Position" 
                stroke="#387908" 
                dot={false} 
                activeDot={{ r: 8 }}
              />
            </LineChart>
          </ResponsiveContainer>
        </div>
      )}

      {activeTab === 'speed' && (
        <div className="mb-8">
          <h3 className="text-lg font-semibold mb-2">Horizontal Speed of Head and Tail Over Time</h3>
          <ResponsiveContainer width="100%" height={400}>
            <LineChart data={formattedData} margin={{ top: 5, right: 30, left: 20, bottom: 5 }}>
              <CartesianGrid strokeDasharray="3 3" />
              <XAxis 
                dataKey="displayTime" 
                label={{ value: getTimeLabel(), position: 'insideBottomRight', offset: -10 }} 
              />
              <YAxis 
                label={{ value: 'Horizontal Speed (cm/s)', angle: -90, position: 'insideLeft' }} 
              />
              <Tooltip 
                formatter={(value) => [value.toFixed(2) + ' cm/s', '']}
                labelFormatter={(value) => `Time: ${value} ${timeUnit}`}
              />
              <Legend />
              <ReferenceLine y={0} stroke="#000" strokeDasharray="3 3" />
              <Line 
                type="monotone" 
                dataKey="headSpeed" 
                name="Head Speed" 
                stroke="#ff7300" 
                dot={false} 
                activeDot={{ r: 8 }}
              />
              <Line 
                type="monotone" 
                dataKey="tailSpeed" 
                name="Tail Speed" 
                stroke="#387908" 
                dot={false} 
                activeDot={{ r: 8 }}
              />
            </LineChart>
          </ResponsiveContainer>
        </div>
      )}

      {activeTab === 'distance' && (
        <div className="mb-8">
          <h3 className="text-lg font-semibold mb-2">Vertical Distance Between Head and Tail</h3>
          <ResponsiveContainer width="100%" height={400}>
            <LineChart data={formattedData} margin={{ top: 5, right: 30, left: 20, bottom: 5 }}>
              <CartesianGrid strokeDasharray="3 3" />
              <XAxis 
                dataKey="displayTime" 
                label={{ value: getTimeLabel(), position: 'insideBottomRight', offset: -10 }} 
              />
              <YAxis 
                label={{ value: 'Distance (cm)', angle: -90, position: 'insideLeft' }} 
              />
              <Tooltip 
                formatter={(value) => [value.toFixed(2) + ' cm', '']}
                labelFormatter={(value) => `Time: ${value} ${timeUnit}`}
              />
              <Legend />
              <ReferenceLine y={0} stroke="#000" strokeDasharray="3 3" />
              <Line 
                type="monotone" 
                dataKey="verticalDistance" 
                name="Head-Tail Distance" 
                stroke="#8884d8" 
                dot={false} 
                activeDot={{ r: 8 }}
              />
            </LineChart>
          </ResponsiveContainer>
        </div>
      )}

      <div className="mt-8 bg-gray-100 p-4 rounded">
        <h3 className="text-lg font-semibold mb-2">Data Summary</h3>
        <div className="grid grid-cols-1 md:grid-cols-2 gap-4">
          <div>
            <h4 className="font-medium">Vertical Position (cm)</h4>
            <ul className="list-disc pl-5 mt-2">
              <li>Head: Range from {headVerticalRange.min.toFixed(1)} to {headVerticalRange.max.toFixed(1)} cm</li>
              <li>Tail: Range from {tailVerticalRange.min.toFixed(1)} to {tailVerticalRange.max.toFixed(1)} cm</li>
              <li>Average Head Position: {(data.reduce((sum, d) => sum + d.headVertical, 0) / data.length).toFixed(1)} cm</li>
              <li>Average Tail Position: {(data.reduce((sum, d) => sum + d.tailVertical, 0) / data.length).toFixed(1)} cm</li>
            </ul>
          </div>
          <div>
            <h4 className="font-medium">Horizontal Speed (cm/s)</h4>
            <ul className="list-disc pl-5 mt-2">
              <li>Head: Range from {Math.min(...data.map(d => d.headSpeed)).toFixed(1)} to {Math.max(...data.map(d => d.headSpeed)).toFixed(1)} cm/s</li>
              <li>Tail: Range from {Math.min(...data.map(d => d.tailSpeed)).toFixed(1)} to {Math.max(...data.map(d => d.tailSpeed)).toFixed(1)} cm/s</li>
              <li>Average Head Speed: {(data.reduce((sum, d) => sum + d.headSpeed, 0) / data.length).toFixed(2)} cm/s</li>
              <li>Average Tail Speed: {(data.reduce((sum, d) => sum + d.tailSpeed, 0) / data.length).toFixed(2)} cm/s</li>
            </ul>
          </div>
        </div>
        <div className="mt-4">
          <h4 className="font-medium">Head-Tail Relationship</h4>
          <ul className="list-disc pl-5 mt-2">
            <li>Maximum Distance: {maxVerticalDistance.toFixed(1)} cm</li>
            <li>Average Distance: {Math.abs(data.reduce((sum, d) => sum + d.verticalDistance, 0) / data.length).toFixed(1)} cm</li>
          </ul>
        </div>
      </div>
    </div>
  );
};

export default RobotSnakeVisualization;
