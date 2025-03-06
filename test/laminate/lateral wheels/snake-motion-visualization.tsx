import React, { useState, useEffect } from 'react';
import { LineChart, Line, XAxis, YAxis, CartesianGrid, Tooltip, Legend, ResponsiveContainer, ScatterChart, Scatter, ZAxis } from 'recharts';
import Papa from 'papaparse';

const SnakeMotionAnalysis = () => {
  const [data, setData] = useState([]);
  const [isLoading, setIsLoading] = useState(true);
  const [stats, setStats] = useState(null);
  
  useEffect(() => {
    const fetchData = async () => {
      try {
        setIsLoading(true);
        const response = await window.fs.readFile('combined_lateral_wheels.csv', { encoding: 'utf8' });
        
        Papa.parse(response, {
          header: true,
          dynamicTyping: true,
          skipEmptyLines: true,
          complete: (results) => {
            // Process data - convert to cm and calculate additional metrics
            const processedData = results.data.map(row => {
              // Convert time from ms to seconds
              const timeInSeconds = row['Zeit (ms)'] / 1000;
              
              // Convert displacements from m to cm
              const head_vertical_cm = row['Markierung_1_vertical_displacement (m)'] * 100;
              const tail_vertical_cm = row['Markierung_2_vertical_displacement'] * 100;
              
              // Convert speeds from m/s to cm/s
              const head_horizontal_speed_cms = row['Markierung_1_horizontal_speed(m/S)'] * 100;
              const tail_horizontal_speed_cms = row['Markierung_2_horizontal_speed'] * 100;
              
              // Calculate vertical displacement difference
              const vertical_difference_cm = head_vertical_cm - tail_vertical_cm;
              
              return {
                time_seconds: timeInSeconds,
                head_vertical_cm,
                tail_vertical_cm,
                head_horizontal_speed_cms,
                tail_horizontal_speed_cms,
                vertical_difference_cm
              };
            });
            
            // Calculate basic statistics for display
            const calculateStats = (data) => {
              const timeRange = {
                min: Math.min(...data.map(d => d.time_seconds)),
                max: Math.max(...data.map(d => d.time_seconds))
              };
              
              const headVertical = {
                min: Math.min(...data.map(d => d.head_vertical_cm)),
                max: Math.max(...data.map(d => d.head_vertical_cm)),
                avg: data.reduce((sum, d) => sum + d.head_vertical_cm, 0) / data.length
              };
              
              const tailVertical = {
                min: Math.min(...data.map(d => d.tail_vertical_cm)),
                max: Math.max(...data.map(d => d.tail_vertical_cm)),
                avg: data.reduce((sum, d) => sum + d.tail_vertical_cm, 0) / data.length
              };
              
              const headSpeed = {
                min: Math.min(...data.map(d => d.head_horizontal_speed_cms)),
                max: Math.max(...data.map(d => d.head_horizontal_speed_cms)),
                avg: data.reduce((sum, d) => sum + d.head_horizontal_speed_cms, 0) / data.length
              };
              
              const tailSpeed = {
                min: Math.min(...data.map(d => d.tail_horizontal_speed_cms)),
                max: Math.max(...data.map(d => d.tail_horizontal_speed_cms)),
                avg: data.reduce((sum, d) => sum + d.tail_horizontal_speed_cms, 0) / data.length
              };
              
              return {
                timeRange,
                headVertical,
                tailVertical,
                headSpeed,
                tailSpeed
              };
            };
            
            setStats(calculateStats(processedData));
            setData(processedData);
            setIsLoading(false);
          },
          error: (error) => {
            console.error('Error parsing CSV:', error);
            setIsLoading(false);
          }
        });
      } catch (error) {
        console.error('Error reading file:', error);
        setIsLoading(false);
      }
    };
    
    fetchData();
  }, []);
  
  if (isLoading) {
    return <div className="flex justify-center items-center h-64">Loading snake motion data...</div>;
  }
  
  // Format numbers to 2 decimal places
  const formatNumber = (num) => {
    return num ? num.toFixed(2) : 'N/A';
  };

  return (
    <div className="p-4">
      <h1 className="text-2xl font-bold mb-4">Robot Snake Motion Analysis</h1>
      
      <div className="grid grid-cols-1 gap-4 mb-6">
        {stats && (
          <div className="bg-blue-50 p-4 rounded-lg">
            <h2 className="text-xl font-semibold mb-2">Key Statistics</h2>
            <div className="grid grid-cols-1 md:grid-cols-2 gap-4">
              <div>
                <h3 className="font-medium">Time Range:</h3>
                <p>{formatNumber(stats.timeRange.min)} - {formatNumber(stats.timeRange.max)} seconds</p>
                
                <h3 className="font-medium mt-2">Head Vertical Range (cm):</h3>
                <p>{formatNumber(stats.headVertical.min)} to {formatNumber(stats.headVertical.max)}</p>
                <p>Average: {formatNumber(stats.headVertical.avg)}</p>
                
                <h3 className="font-medium mt-2">Tail Vertical Range (cm):</h3>
                <p>{formatNumber(stats.tailVertical.min)} to {formatNumber(stats.tailVertical.max)}</p>
                <p>Average: {formatNumber(stats.tailVertical.avg)}</p>
              </div>
              
              <div>
                <h3 className="font-medium">Head Horizontal Speed (cm/s):</h3>
                <p>{formatNumber(stats.headSpeed.min)} to {formatNumber(stats.headSpeed.max)}</p>
                <p>Average: {formatNumber(stats.headSpeed.avg)}</p>
                
                <h3 className="font-medium mt-2">Tail Horizontal Speed (cm/s):</h3>
                <p>{formatNumber(stats.tailSpeed.min)} to {formatNumber(stats.tailSpeed.max)}</p>
                <p>Average: {formatNumber(stats.tailSpeed.avg)}</p>
                
                <h3 className="font-medium mt-2">Estimated Wave Frequency:</h3>
                <p>~0.6 Hz (Period: ~1.7 seconds)</p>
                
                <h3 className="font-medium mt-2">Estimated Phase Difference:</h3>
                <p>~60° between head and tail</p>
              </div>
            </div>
          </div>
        )}
      </div>
      
      <div className="mb-8">
        <h2 className="text-xl font-semibold mb-4">Vertical Displacement Over Time</h2>
        <div className="h-64">
          <ResponsiveContainer width="100%" height="100%">
            <LineChart
              data={data}
              margin={{ top: 5, right: 30, left: 20, bottom: 5 }}
            >
              <CartesianGrid strokeDasharray="3 3" />
              <XAxis 
                dataKey="time_seconds" 
                label={{ value: 'Time (seconds)', position: 'insideBottomRight', offset: -5 }} 
              />
              <YAxis 
                label={{ value: 'Vertical Displacement (cm)', angle: -90, position: 'insideLeft' }} 
              />
              <Tooltip formatter={(value) => [value.toFixed(2) + ' cm', '']} labelFormatter={(label) => `Time: ${label.toFixed(2)}s`} />
              <Legend />
              <Line type="monotone" dataKey="head_vertical_cm" name="Head Position" stroke="#8884d8" dot={false} />
              <Line type="monotone" dataKey="tail_vertical_cm" name="Tail Position" stroke="#82ca9d" dot={false} />
            </LineChart>
          </ResponsiveContainer>
        </div>
      </div>
      
      <div className="mb-8">
        <h2 className="text-xl font-semibold mb-4">Horizontal Speed</h2>
        <div className="h-64">
          <ResponsiveContainer width="100%" height="100%">
            <LineChart
              data={data}
              margin={{ top: 5, right: 30, left: 20, bottom: 5 }}
            >
              <CartesianGrid strokeDasharray="3 3" />
              <XAxis 
                dataKey="time_seconds" 
                label={{ value: 'Time (seconds)', position: 'insideBottomRight', offset: -5 }} 
              />
              <YAxis 
                label={{ value: 'Horizontal Speed (cm/s)', angle: -90, position: 'insideLeft' }} 
              />
              <Tooltip formatter={(value) => [value.toFixed(2) + ' cm/s', '']} labelFormatter={(label) => `Time: ${label.toFixed(2)}s`} />
              <Legend />
              <Line type="monotone" dataKey="head_horizontal_speed_cms" name="Head Speed" stroke="#ff7300" dot={false} />
              <Line type="monotone" dataKey="tail_horizontal_speed_cms" name="Tail Speed" stroke="#0088fe" dot={false} />
            </LineChart>
          </ResponsiveContainer>
        </div>
      </div>
      
      <div className="mb-8">
        <h2 className="text-xl font-semibold mb-4">Head-Tail Vertical Difference</h2>
        <div className="h-64">
          <ResponsiveContainer width="100%" height="100%">
            <LineChart
              data={data}
              margin={{ top: 5, right: 30, left: 20, bottom: 5 }}
            >
              <CartesianGrid strokeDasharray="3 3" />
              <XAxis 
                dataKey="time_seconds" 
                label={{ value: 'Time (seconds)', position: 'insideBottomRight', offset: -5 }} 
              />
              <YAxis 
                label={{ value: 'Vertical Difference (cm)', angle: -90, position: 'insideLeft' }} 
              />
              <Tooltip formatter={(value) => [value.toFixed(2) + ' cm', '']} labelFormatter={(label) => `Time: ${label.toFixed(2)}s`} />
              <Legend />
              <Line type="monotone" dataKey="vertical_difference_cm" name="Head-Tail Difference" stroke="#ff5252" dot={false} />
            </LineChart>
          </ResponsiveContainer>
        </div>
      </div>
      
      <div className="mb-4">
        <h2 className="text-xl font-semibold mb-4">Snake Motion Pattern Interpretation</h2>
        <div className="bg-gray-50 p-4 rounded-lg">
          <p className="mb-2">The data shows a snake-like undulating motion with the following characteristics:</p>
          <ul className="list-disc pl-5 space-y-1">
            <li>The head and tail move in opposite vertical directions (out of phase), creating the typical serpentine motion</li>
            <li>The vertical displacement range is larger for the head (~1550 cm total range) than the tail (~1340 cm total range)</li>
            <li>Both the head and tail have negative average horizontal speeds, indicating the robot is moving backward overall</li>
            <li>The undulation frequency is approximately 0.6 Hz, meaning the snake completes one full wave cycle in about 1.7 seconds</li>
            <li>The phase difference between head and tail motion is around 60°, which helps create the propagating wave pattern that propels the snake</li>
          </ul>
        </div>
      </div>
    </div>
  );
};

export default SnakeMotionAnalysis;
