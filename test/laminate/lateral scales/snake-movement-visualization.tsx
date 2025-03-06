import React, { useState, useEffect } from 'react';
import { LineChart, Line, XAxis, YAxis, CartesianGrid, Tooltip, Legend, ResponsiveContainer, ScatterChart, Scatter, ZAxis } from 'recharts';
import Papa from 'papaparse';

const SnakeMovementAnalysis = () => {
  const [data, setData] = useState([]);
  const [loading, setLoading] = useState(true);
  const [error, setError] = useState(null);
  const [activeTab, setActiveTab] = useState('displacement');
  
  useEffect(() => {
    const fetchData = async () => {
      try {
        setLoading(true);
        const response = await window.fs.readFile('combined_lateral_scales.csv', { encoding: 'utf8' });
        
        Papa.parse(response, {
          header: true,
          dynamicTyping: true,
          skipEmptyLines: true,
          delimitersToGuess: [',', ';', '\t'],
          complete: (results) => {
            // Process the data
            const processedData = results.data.map(row => {
              // Helper function to convert string to number
              const toNum = (val) => {
                if (typeof val === 'string' && val !== '') {
                  // Replace comma with dot for decimal point if needed
                  const stringValue = val.replace(',', '.');
                  return parseFloat(stringValue);
                }
                return val;
              };
              
              return {
                time: row['Zeit (ms)'] / 1000, // Convert to seconds
                headHorizDisp: toNum(row['Markierung 2_horizontal_displacement (m)']) * 100,
                tailHorizDisp: toNum(row['Markierung 1_horizontal_displacement']) * 100,
                headVertDisp: toNum(row['Markierung 2_vertical_displacement']) * 100,
                tailVertDisp: toNum(row['Markierung 1_vertical_displacement']) * 100,
                headHorizSpeed: toNum(row['Markierung 2_horizontal_speed(m/s)']) * 100,
                tailHorizSpeed: toNum(row['Markierung 1_horizontal_speed']) * 100,
                headVertSpeed: toNum(row['Markierung 2_vertical_speed']) * 100,
                tailVertSpeed: toNum(row['Markierung 1_vertical_speed']) * 100
              };
            });
            
            // Sample the data to avoid performance issues
            const samplingRate = Math.max(1, Math.floor(processedData.length / 200));
            const sampledData = [];
            for (let i = 0; i < processedData.length; i += samplingRate) {
              sampledData.push(processedData[i]);
            }
            
            setData(sampledData);
            setLoading(false);
          },
          error: (error) => {
            setError(`Error parsing CSV: ${error}`);
            setLoading(false);
          }
        });
      } catch (error) {
        setError(`Error fetching data: ${error.message}`);
        setLoading(false);
      }
    };
    
    fetchData();
  }, []);
  
  // Calculate statistics
  const calculateStats = (dataArray, key) => {
    const values = dataArray.map(item => item[key]).filter(val => !isNaN(val));
    if (values.length === 0) return { min: 'N/A', max: 'N/A', avg: 'N/A', range: 'N/A' };
    
    const min = Math.min(...values);
    const max = Math.max(...values);
    return {
      min: min.toFixed(2),
      max: max.toFixed(2),
      avg: (values.reduce((a, b) => a + b, 0) / values.length).toFixed(2),
      range: (max - min).toFixed(2)
    };
  };
  
  // Statistics for different measurements
  const stats = {
    headHorizDisp: calculateStats(data, 'headHorizDisp'),
    tailHorizDisp: calculateStats(data, 'tailHorizDisp'),
    headVertDisp: calculateStats(data, 'headVertDisp'),
    tailVertDisp: calculateStats(data, 'tailVertDisp')
  };
  
  if (loading) {
    return <div className="p-4 text-center">Loading data...</div>;
  }
  
  if (error) {
    return <div className="p-4 text-red-500">{error}</div>;
  }
  
  return (
    <div className="p-4">
      <h1 className="text-2xl font-bold mb-4">Robot Snake Movement Analysis</h1>
      
      <div className="mb-6">
        <div className="flex border-b">
          <button 
            className={`px-4 py-2 ${activeTab === 'displacement' ? 'bg-blue-500 text-white' : 'bg-gray-200'}`}
            onClick={() => setActiveTab('displacement')}
          >
            Displacement
          </button>
          <button 
            className={`px-4 py-2 ${activeTab === 'trajectory' ? 'bg-blue-500 text-white' : 'bg-gray-200'}`}
            onClick={() => setActiveTab('trajectory')}
          >
            Trajectory
          </button>
          <button 
            className={`px-4 py-2 ${activeTab === 'speed' ? 'bg-blue-500 text-white' : 'bg-gray-200'}`}
            onClick={() => setActiveTab('speed')}
          >
            Speed
          </button>
          <button 
            className={`px-4 py-2 ${activeTab === 'stats' ? 'bg-blue-500 text-white' : 'bg-gray-200'}`}
            onClick={() => setActiveTab('stats')}
          >
            Statistics
          </button>
        </div>
      </div>
      
      {activeTab === 'displacement' && (
        <div>
          <h2 className="text-xl font-semibold mb-2">Horizontal Displacement Over Time (cm)</h2>
          <div className="h-64 mb-8">
            <ResponsiveContainer width="100%" height="100%">
              <LineChart data={data}>
                <CartesianGrid strokeDasharray="3 3" />
                <XAxis 
                  dataKey="time" 
                  label={{ value: 'Time (seconds)', position: 'insideBottom', offset: -5 }} 
                />
                <YAxis 
                  label={{ value: 'Horizontal Displacement (cm)', angle: -90, position: 'insideLeft' }} 
                />
                <Tooltip formatter={(value) => `${value.toFixed(2)} cm`} />
                <Legend />
                <Line 
                  type="monotone" 
                  dataKey="headHorizDisp" 
                  name="Head" 
                  stroke="#8884d8" 
                  dot={false} 
                  activeDot={{ r: 8 }} 
                />
                <Line 
                  type="monotone" 
                  dataKey="tailHorizDisp" 
                  name="Tail" 
                  stroke="#82ca9d" 
                  dot={false} 
                  activeDot={{ r: 8 }} 
                />
              </LineChart>
            </ResponsiveContainer>
          </div>
          
          <h2 className="text-xl font-semibold mb-2">Vertical Displacement Over Time (cm)</h2>
          <div className="h-64">
            <ResponsiveContainer width="100%" height="100%">
              <LineChart data={data}>
                <CartesianGrid strokeDasharray="3 3" />
                <XAxis 
                  dataKey="time" 
                  label={{ value: 'Time (seconds)', position: 'insideBottom', offset: -5 }} 
                />
                <YAxis 
                  label={{ value: 'Vertical Displacement (cm)', angle: -90, position: 'insideLeft' }} 
                />
                <Tooltip formatter={(value) => `${value.toFixed(2)} cm`} />
                <Legend />
                <Line 
                  type="monotone" 
                  dataKey="headVertDisp" 
                  name="Head" 
                  stroke="#8884d8" 
                  dot={false} 
                  activeDot={{ r: 8 }} 
                />
                <Line 
                  type="monotone" 
                  dataKey="tailVertDisp" 
                  name="Tail" 
                  stroke="#82ca9d" 
                  dot={false} 
                  activeDot={{ r: 8 }} 
                />
              </LineChart>
            </ResponsiveContainer>
          </div>
        </div>
      )}
      
      {activeTab === 'trajectory' && (
        <div>
          <h2 className="text-xl font-semibold mb-2">Snake Movement Trajectory</h2>
          <div className="h-96">
            <ResponsiveContainer width="100%" height="100%">
              <ScatterChart>
                <CartesianGrid />
                <XAxis 
                  type="number" 
                  dataKey="headHorizDisp" 
                  name="Horizontal Displacement" 
                  label={{ value: 'Horizontal Displacement (cm)', position: 'insideBottom', offset: -5 }} 
                />
                <YAxis 
                  type="number" 
                  dataKey="headVertDisp" 
                  name="Vertical Displacement" 
                  label={{ value: 'Vertical Displacement (cm)', angle: -90, position: 'insideLeft' }} 
                />
                <ZAxis range={[20, 20]} />
                <Tooltip cursor={{ strokeDasharray: '3 3' }} formatter={(value) => `${value.toFixed(2)} cm`} />
                <Legend />
                <Scatter name="Head Position" data={data} fill="#8884d8" />
                <Scatter name="Tail Position" data={data.map(item => ({
                  ...item,
                  headHorizDisp: item.tailHorizDisp,
                  headVertDisp: item.tailVertDisp
                }))} fill="#82ca9d" />
              </ScatterChart>
            </ResponsiveContainer>
          </div>
        </div>
      )}
      
      {activeTab === 'speed' && (
        <div>
          <h2 className="text-xl font-semibold mb-2">Horizontal Speed Over Time (cm/s)</h2>
          <div className="h-64 mb-8">
            <ResponsiveContainer width="100%" height="100%">
              <LineChart data={data}>
                <CartesianGrid strokeDasharray="3 3" />
                <XAxis 
                  dataKey="time" 
                  label={{ value: 'Time (seconds)', position: 'insideBottom', offset: -5 }} 
                />
                <YAxis 
                  label={{ value: 'Horizontal Speed (cm/s)', angle: -90, position: 'insideLeft' }} 
                />
                <Tooltip formatter={(value) => `${value.toFixed(2)} cm/s`} />
                <Legend />
                <Line 
                  type="monotone" 
                  dataKey="headHorizSpeed" 
                  name="Head" 
                  stroke="#8884d8" 
                  dot={false} 
                  activeDot={{ r: 8 }} 
                />
                <Line 
                  type="monotone" 
                  dataKey="tailHorizSpeed" 
                  name="Tail" 
                  stroke="#82ca9d" 
                  dot={false} 
                  activeDot={{ r: 8 }} 
                />
              </LineChart>
            </ResponsiveContainer>
          </div>
          
          <h2 className="text-xl font-semibold mb-2">Vertical Speed Over Time (cm/s)</h2>
          <div className="h-64">
            <ResponsiveContainer width="100%" height="100%">
              <LineChart data={data}>
                <CartesianGrid strokeDasharray="3 3" />
                <XAxis 
                  dataKey="time" 
                  label={{ value: 'Time (seconds)', position: 'insideBottom', offset: -5 }} 
                />
                <YAxis 
                  label={{ value: 'Vertical Speed (cm/s)', angle: -90, position: 'insideLeft' }} 
                />
                <Tooltip formatter={(value) => `${value.toFixed(2)} cm/s`} />
                <Legend />
                <Line 
                  type="monotone" 
                  dataKey="headVertSpeed" 
                  name="Head" 
                  stroke="#8884d8" 
                  dot={false} 
                  activeDot={{ r: 8 }} 
                />
                <Line 
                  type="monotone" 
                  dataKey="tailVertSpeed" 
                  name="Tail" 
                  stroke="#82ca9d" 
                  dot={false} 
                  activeDot={{ r: 8 }} 
                />
              </LineChart>
            </ResponsiveContainer>
          </div>
        </div>
      )}
      
      {activeTab === 'stats' && (
        <div>
          <h2 className="text-xl font-semibold mb-4">Statistical Analysis (cm)</h2>
          
          <div className="grid grid-cols-2 gap-4 mb-6">
            <div className="border p-4 rounded shadow">
              <h3 className="font-bold mb-2">Head Horizontal Displacement</h3>
              <p><strong>Min:</strong> {stats.headHorizDisp.min} cm</p>
              <p><strong>Max:</strong> {stats.headHorizDisp.max} cm</p>
              <p><strong>Average:</strong> {stats.headHorizDisp.avg} cm</p>
              <p><strong>Range:</strong> {stats.headHorizDisp.range} cm</p>
            </div>
            
            <div className="border p-4 rounded shadow">
              <h3 className="font-bold mb-2">Tail Horizontal Displacement</h3>
              <p><strong>Min:</strong> {stats.tailHorizDisp.min} cm</p>
              <p><strong>Max:</strong> {stats.tailHorizDisp.max} cm</p>
              <p><strong>Average:</strong> {stats.tailHorizDisp.avg} cm</p>
              <p><strong>Range:</strong> {stats.tailHorizDisp.range} cm</p>
            </div>
            
            <div className="border p-4 rounded shadow">
              <h3 className="font-bold mb-2">Head Vertical Displacement</h3>
              <p><strong>Min:</strong> {stats.headVertDisp.min} cm</p>
              <p><strong>Max:</strong> {stats.headVertDisp.max} cm</p>
              <p><strong>Average:</strong> {stats.headVertDisp.avg} cm</p>
              <p><strong>Range:</strong> {stats.headVertDisp.range} cm</p>
            </div>
            
            <div className="border p-4 rounded shadow">
              <h3 className="font-bold mb-2">Tail Vertical Displacement</h3>
              <p><strong>Min:</strong> {stats.tailVertDisp.min} cm</p>
              <p><strong>Max:</strong> {stats.tailVertDisp.max} cm</p>
              <p><strong>Average:</strong> {stats.tailVertDisp.avg} cm</p>
              <p><strong>Range:</strong> {stats.tailVertDisp.range} cm</p>
            </div>
          </div>
          
          <div className="border p-4 rounded shadow">
            <h3 className="font-bold mb-2">Key Observations</h3>
            <ul className="list-disc pl-5">
              <li>The snake's head shows a wider range of horizontal movement compared to the tail.</li>
              <li>Vertical displacement patterns suggest an undulating motion pattern typical of snake locomotion.</li>
              <li>The time series data covers approximately {(data[data.length-1]?.time - data[0]?.time).toFixed(2)} seconds of movement.</li>
              <li>The phase difference between head and tail movements indicates the propagation of waves along the snake's body.</li>
            </ul>
          </div>
        </div>
      )}
    </div>
  );
};

export default SnakeMovementAnalysis;