import React from "react";
import { createRoot } from "react-dom/client";
import "./styles.css";
import App from "./App";

// Create a root
const rootElement = document.getElementById("root");
const root = createRoot(rootElement);

// Render your app
root.render(
  <React.StrictMode>
    <App />
  </React.StrictMode>
);
