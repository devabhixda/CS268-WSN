import React, { useEffect, useState } from "react";
import { database, ref, onValue } from "../firebase";
import { Line } from "react-chartjs-2";
import "../chartConfig";
import "./Dashboard.css";

const Dashboard = () => {
  const [networkStatus, setNetworkStatus] = useState({});
  const [sensorData, setSensorData] = useState({});
  const [timeFilter, setTimeFilter] = useState(60); // Default filter (60 mins)
  const [nodeFilter, setNodeFilter] = useState([]); // Selected nodes for the graph
  const [viewMode, setViewMode] = useState("mean"); // Default view is "Mean View"

  useEffect(() => {
    const networkStatusRef = ref(database, "network_status");
    onValue(networkStatusRef, (snapshot) => {
      setNetworkStatus(snapshot.val());
    });

    const sensorDataRef = ref(database, "sensor_data");
    onValue(sensorDataRef, (snapshot) => {
      const data = snapshot.val();
      setSensorData(data);
      setNodeFilter(Object.keys(data || [])); // Initialize with all nodes
    });
  }, []);

  const filteredSensorData = (nodeId) => {
    const now = Math.floor(Date.now() / 1000);
    if (timeFilter === "all") {
      // No filtering for "All Time"
      return sensorData[nodeId] || {};
    }
    const timeLimit = now - timeFilter * 60; // Convert filter to seconds
    return Object.entries(sensorData[nodeId] || {})
      .filter(([timestamp]) => parseInt(timestamp) >= timeLimit)
      .reduce((acc, [timestamp, value]) => ({ ...acc, [timestamp]: value }), {});
  };

  const calculateMeanData = () => {
    const nodeData = nodeFilter.map((nodeId) => filteredSensorData(nodeId));
    const timestamps = new Set(
      nodeData.flatMap((data) => Object.keys(data))
    );

    // Calculate the mean for each timestamp
    const meanData = {};
    timestamps.forEach((timestamp) => {
      const values = nodeData
        .map((data) => parseFloat(data[timestamp]?.value || 0))
        .filter((value) => value > 0); // Ignore 0 or undefined values
      if (values.length > 0) {
        meanData[timestamp] =
          values.reduce((sum, val) => sum + val, 0) / values.length;
      }
    });
    return meanData;
  };

  const handleNodeFilterChange = (e) => {
    const { value, checked } = e.target;
    setNodeFilter((prev) =>
      checked ? [...prev, value] : prev.filter((nodeId) => nodeId !== value)
    );
  };

  const renderGraphData = () => {
    if (viewMode === "mean") {
      const meanData = calculateMeanData();
      return {
        labels: Object.keys(meanData).map((timestamp) =>
          new Date(parseInt(timestamp) * 1000).toLocaleTimeString()
        ),
        datasets: [
          {
            label: "Mean Sensor Value",
            data: Object.values(meanData),
            borderColor: "rgb(75, 192, 192)",
            backgroundColor: "rgba(75, 192, 192, 0.2)",
            fill: true,
          },
        ],
      };
    }

    // Individual node data
    return {
      labels: Object.keys(filteredSensorData(Object.keys(sensorData)[0] || {})).map((timestamp) =>
        new Date(parseInt(timestamp) * 1000).toLocaleTimeString()
      ),
      datasets: nodeFilter.map((nodeId, index) => ({
        label: `Node ${nodeId}`,
        data: Object.values(filteredSensorData(nodeId)).map(({ value }) => value),
        borderColor: `hsl(${(index * 50) % 360}, 70%, 50%)`,
        fill: false,
      })),
    };
  };

  return (
    <div className="dashboard-container">
      <h1 className="dashboard-title">Network Dashboard</h1>

      <div className="network-status">
        <h2>Network Status</h2>
        <p>Active Nodes: {networkStatus.active_nodes}</p>
        <p>Current Parent: {networkStatus.current_parent}</p>
        <p>Last Update: {new Date(networkStatus.last_update * 1000).toLocaleString()}</p>
      </div>

      <div className="filter-controls">
        <label htmlFor="timeFilter">Time Filter:</label>
        <select
          id="timeFilter"
          value={timeFilter}
          onChange={(e) => setTimeFilter(e.target.value)}
        >
          <option value={5}>Last 5 minutes</option>
          <option value={10}>Last 10 minutes</option>
          <option value={30}>Last 30 minutes</option>
          <option value={60}>Last 60 minutes</option>
          <option value="all">All Time</option>
        </select>
      </div>

      <div className="node-filter">
        <h3>Filter by Node:</h3>
        {Object.keys(sensorData || {}).map((nodeId) => (
          <div key={nodeId} className="node-checkbox">
            <input
              type="checkbox"
              id={`node-${nodeId}`}
              value={nodeId}
              checked={nodeFilter.includes(nodeId)}
              onChange={handleNodeFilterChange}
            />
            <label htmlFor={`node-${nodeId}`}>Node {nodeId}</label>
          </div>
        ))}
      </div>

      <div className="view-mode">
        <label>
          <input
            type="radio"
            value="mean"
            checked={viewMode === "mean"}
            onChange={(e) => setViewMode(e.target.value)}
          />
          Mean View
        </label>
        <label>
          <input
            type="radio"
            value="individual"
            checked={viewMode === "individual"}
            onChange={(e) => setViewMode(e.target.value)}
          />
          Individual Nodes
        </label>
      </div>

      <div className="sensor-data">
        <h2>Sensor Data</h2>
        <Line data={renderGraphData()} />
      </div>
    </div>
  );
};

export default Dashboard;
