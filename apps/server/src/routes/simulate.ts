import { Router } from 'express';
import { z } from 'zod';
import { simulate } from '../engine';
import type { SimulationInput } from '../engine/types';

const router = Router();

const edgeSchema = z.object({
  id: z.string(),
  source: z.string(),
  target: z.string(),
  sourceHandle: z.string().optional(),
  targetHandle: z.string().optional()
});

const nodeSchema = z.object({
  id: z.string(),
  type: z.string(),
  data: z.record(z.any())
});

const simulationSchema = z.object({
  graph: z.object({
    nodes: z.array(nodeSchema),
    edges: z.array(edgeSchema)
  }),
  families: z.array(z.string()).min(1),
  nSteps: z.number().min(1)
});

router.post('/', (req, res, next) => {
  try {
    const payload = simulationSchema.parse(req.body);
    const input: SimulationInput = {
      graph: {
        nodes: payload.graph.nodes as any,
        edges: payload.graph.edges
      },
      families: payload.families,
      nSteps: payload.nSteps
    };
    const result = simulate(input);
    res.json(result);
  } catch (error) {
    next(error);
  }
});

export default router;
