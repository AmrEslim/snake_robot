import React, { useState, useEffect } from 'react';
import { LineChart, Line, XAxis, YAxis, CartesianGrid, Tooltip, Legend, ResponsiveContainer, ScatterChart, Scatter, ZAxis } from 'recharts';
import Papa from 'papaparse';

const SnakeRobotAnalysis = () => {
  const [data, setData] = useState([]);
  const [loading, setLoading] = useState(true);
  const [stats, setStats] = useState({});

  useEffect(() => {
    const fetchData = async () => {
      try {
        const response = await window.fs.readFile('combined_sidewinding_base.csv', { encoding: 'utf8' });
        const parsedData = Papa.parse(response, {
          header: true,
          dynamicTyping: true,
          skipEmptyLines: true
        });

        // Convert data to cm and prepare for visualization
        const processedData = parsedData.data.map(row => {
          return {
            time_ms: row['Zeit (ms)'],
            head_horizontal_cm: row['Markierung 1_horizontal_deviation(m)'] * 100, // m to cm
            tail_horizontal_cm: row['Markierung 2_horizontal_deviation(m)'] * 100, // m to cm
            head_vertical_speed_cm_s: row['Markierung 1_vertical_speed (m/s)'] * 100, // m/s to cm/s
            tail_vertical_speed_cm_s: row['Markierung 2_vertical_speed (m/s)'] * 100, // m/s to cm/s
            phase_difference: row['Markierung 1_horizontal_deviation(m)'] * 100 - row['Markierung 2_horizontal_deviation(m)'] * 100
          };
        });

        // Calculate some basic statistics
        const calculateStats = (array) => {
          const min = Math.min(...array);
          const max = Math.max(...array);
          const sum = array.reduce((a, b) => a + b, 0);
          const avg = sum / array.length;
          return { min: min.toFixed(2), max: max.toFixed(2), avg: avg.toFixed(2) };
        };

        const calculatedStats = {
          headHorizontal: calculateStats(processedData.map(d => d.head_horizontal_cm)),
          tailHorizontal: calculateStats(processedData.map(d => d.tail_horizontal_cm)),
          headVerticalSpeed: calculateStats(processedData.map(d => d.head_vertical_speed_cm_s)),
          tailVerticalSpeed: calculateStats(processedData.map(d => d.tail_vertical_speed_cm_s)),
          phaseDifference: calculateStats(processedData.map(d => d.phase_difference))
        };

        setData(processedData);
        setStats(calculatedStats);
        setLoading(false);
      } catch (error) {
        console.error('Error reading file:', error);
        setLoading(false);
      }
    };

    fetchData();
  }, []);

  if (loading) {
    return <div className="p-4 text-center">Loading snake robot data...</div>;
  }

  return (
    <div className="p-4">
      <h1 className="text-2xl font-bold mb-4">Snake Robot Movement Analysis</h1>
      
      <div className="mb-8 p-4 bg-gray-100 rounded">
        <h2 className="text-xl font-semibold mb-2">Key Statistics (in cm and cm/s)</h2>
        
        <div className="grid grid-cols-1 md:grid-cols-2 gap-4">
          <div className="p-3 bg-white rounded shadow">
            <h3 className="font-medium text-lg mb-2">Horizontal Deviation</h3>
            <p><strong>Head:</strong> Min: {stats.headHorizontal?.min} cm, Max: {stats.headHorizontal?.max} cm, Avg: {stats.headHorizontal?.avg} cm</p>
            <p><strong>Tail:</strong> Min: {stats.tailHorizontal?.min} cm, Max: {stats.tailHorizontal?.max} cm, Avg: {stats.tailHorizontal?.avg} cm</p>
          </div>
          
          <div className="p-3 bg-white rounded shadow">
            <h3 className="font-medium text-lg mb-2">Vertical Speed</h3>
            <p><strong>Head:</strong> Min: {stats.headVerticalSpeed?.min} cm/s, Max: {stats.headVerticalSpeed?.max} cm/s, Avg: {stats.headVerticalSpeed?.avg} cm/s</p>
            <p><strong>Tail:</strong> Min: {stats.tailVerticalSpeed?.min} cm/s, Max: {stats.tailVerticalSpeed?.max} cm/s, Avg: {stats.tailVerticalSpeed?.avg} cm/s</p>
          </div>
        </div>
        
        <div className="mt-4 p-3 bg-white rounded shadow">
          <h3 className="font-medium text-lg mb-2">Phase Relationship</h3>
          <p><strong>Head-Tail Difference:</strong> Min: {stats.phaseDifference?.min} cm, Max: {stats.phaseDifference?.max} cm, Avg: {stats.phaseDifference?.avg} cm</p>
          <p className="mt-2 text-sm text-gray-600">Note: Positive values indicate the head is further right than the tail in horizontal position</p>
        </div>
      </div>

      <div className="mb-8">
        <h2 className="text-xl font-semibold mb-4">Horizontal Deviation Over Time</h2>
        <div className="h-64 md:h-80">
          <ResponsiveContainer width="100%" height="100%">
            <LineChart data={data} margin={{ top: 5, right: 20, left: 20, bottom: 5 }}>
              <CartesianGrid strokeDasharray="3 3" />
              <XAxis 
                dataKey="time_ms" 
                label={{ value: 'Time (ms)', position: 'insideBottomRight', offset: -5 }} 
              />
              <YAxis 
                label={{ value: 'Horizontal Deviation (cm)', angle: -90, position: 'insideLeft' }} 
              />
              <Tooltip formatter={(value) => `${value.toFixed(2)} cm`} />
              <Legend />
              <Line type="monotone" dataKey="head_horizontal_cm" name="Head Position" stroke="#8884d8" dot={false} />
              <Line type="monotone" dataKey="tail_horizontal_cm" name="Tail Position" stroke="#82ca9d" dot={false} />
            </LineChart>
          </ResponsiveContainer>
        </div>
      </div>

      <div className="mb-8">
        <h2 className="text-xl font-semibold mb-4">Vertical Speed Over Time</h2>
        <div className="h-64 md:h-80">
          <ResponsiveContainer width="100%" height="100%">
            <LineChart data={data} margin={{ top: 5, right: 20, left: 20, bottom: 5 }}>
              <CartesianGrid strokeDasharray="3 3" />
              <XAxis 
                dataKey="time_ms" 
                label={{ value: 'Time (ms)', position: 'insideBottomRight', offset: -5 }} 
              />
              <YAxis 
                label={{ value: 'Vertical Speed (cm/s)', angle: -90, position: 'insideLeft' }} 
              />
              <Tooltip formatter={(value) => `${value.toFixed(2)} cm/s`} />
              <Legend />
              <Line type="monotone" dataKey="head_vertical_speed_cm_s" name="Head Vertical Speed" stroke="#ff7300" dot={false} />
              <Line type="monotone" dataKey="tail_vertical_speed_cm_s" name="Tail Vertical Speed" stroke="#0088fe" dot={false} />
            </LineChart>
          </ResponsiveContainer>
        </div>
      </div>

      <div className="mb-8">
        <h2 className="text-xl font-semibold mb-4">Phase Relationship (Head vs Tail)</h2>
        <div className="h-64 md:h-80">
          <ResponsiveContainer width="100%" height="100%">
            <ScatterChart margin={{ top: 20, right: 20, bottom: 20, left: 20 }}>
              <CartesianGrid />
              <XAxis 
                type="number" 
                dataKey="head_horizontal_cm" 
                name="Head Position" 
                label={{ value: 'Head Horizontal Position (cm)', position: 'insideBottomRight', offset: -5 }} 
              />
              <YAxis 
                type="number" 
                dataKey="tail_horizontal_cm" 
                name="Tail Position" 
                label={{ value: 'Tail Horizontal Position (cm)', angle: -90, position: 'insideLeft' }} 
              />
              <ZAxis range={[50]} />
              <Tooltip cursor={{ strokeDasharray: '3 3' }} formatter={(value) => `${value.toFixed(2)} cm`} />
              <Scatter name="Positions" data={data} fill="#8884d8" />
            </ScatterChart>
          </ResponsiveContainer>
        </div>
        <p className="text-sm text-gray-600 mt-2">This scatter plot shows the relationship between head and tail horizontal positions, which helps visualize the phase relationship in the sidewinding motion.</p>
      </div>

      <div>
        <h2 className="text-xl font-semibold mb-4">Motion Pattern Analysis</h2>
        <div className="p-4 bg-gray-100 rounded">
          <p className="mb-2"><strong>Sidewinding Motion:</strong> The data shows a characteristic sidewinding pattern with significant phase differences between head and tail movements.</p>
          <p className="mb-2"><strong>Horizontal Movement:</strong> The head's horizontal deviation ranges from {stats.headHorizontal?.min} cm to {stats.headHorizontal?.max} cm, while the tail's ranges from {stats.tailHorizontal?.min} cm to {stats.tailHorizontal?.max} cm.</p>
          <p className="mb-2"><strong>Vertical Speed:</strong> The head's vertical speed ranges from {stats.headVerticalSpeed?.min} cm/s to {stats.headVerticalSpeed?.max} cm/s, while the tail's ranges from {stats.tailVerticalSpeed?.min} cm/s to {stats.tailVerticalSpeed?.max} cm/s.</p>
          <p><strong>Phase Relationship:</strong> The average difference between head and tail positions is {stats.phaseDifference?.avg} cm, indicating the snake robot maintains a consistent phase offset in its sidewinding motion pattern.</p>
        </div>
      </div>
    </div>
  );
};

export default SnakeRobotAnalysis;