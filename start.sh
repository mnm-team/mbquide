#!/bin/bash

trap "echo 'Stopping servers...'; kill $BACKEND_PID $FRONTEND_PID; exit" SIGINT SIGTERM

# Start the backend server
./backend/build/Server &
BACKEND_PID=$!

# Start the frontend server
cd frontend
npm run dev &
FRONTEND_PID=$!

echo "Backend running on http://localhost:18080"
echo "Frontend running on http://localhost:5173/"

wait $BACKEND_PID
wait $FRONTEND_PID