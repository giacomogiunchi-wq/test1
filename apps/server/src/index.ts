import express from 'express';
import cors from 'cors';
import http from 'http';
import { Server as SocketIOServer } from 'socket.io';
import libraryRouter from './routes/library';
import projectsRouter from './routes/projects';
import simulateRouter from './routes/simulate';

const app = express();

app.use(cors());
app.use(express.json({ limit: '5mb' }));

app.use('/api/library', libraryRouter);
app.use('/api/projects', projectsRouter);
app.use('/api/simulate', simulateRouter);

app.use((error: any, _req: express.Request, res: express.Response, _next: express.NextFunction) => {
  console.error(error);
  res.status(500).json({ error: error.message ?? 'Errore interno del server' });
});

const port = process.env.PORT ? Number(process.env.PORT) : 4000;
const server = http.createServer(app);

const io = new SocketIOServer(server, {
  cors: {
    origin: '*'
  }
});

io.on('connection', (socket) => {
  socket.emit('ready');
});

server.listen(port, () => {
  console.log(`Server avviato su http://localhost:${port}`);
});
