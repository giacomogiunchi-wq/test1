import { Router } from 'express';
import { z } from 'zod';
import { loadProject, saveProject, type ProjectPayload } from '../storage';

const router = Router();

const projectSchema = z.object({
  id: z.string().optional(),
  graph: z.unknown(),
  families: z.array(z.string()),
  nSteps: z.number().min(1),
  libraryItems: z.unknown().optional()
});

router.post('/', async (req, res, next) => {
  try {
    const payload = projectSchema.parse(req.body) as ProjectPayload;
    const id = await saveProject(payload);
    res.status(201).json({ id });
  } catch (error) {
    next(error);
  }
});

router.get('/:id', async (req, res, next) => {
  try {
    const project = await loadProject(req.params.id);
    if (!project) {
      res.status(404).json({ error: 'Progetto non trovato' });
      return;
    }
    res.json(project);
  } catch (error) {
    next(error);
  }
});

export default router;
